#include "hexabus_server.hpp"

#include <syslog.h>

#include <libhexabus/filtering.hpp>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "endpoints.h"

extern "C" {
#include <uci.h>
}

using namespace hexadaemon;

namespace hf = hexabus::filtering;
namespace bf = boost::filesystem;

HexabusServer::HexabusServer(boost::asio::io_service& io, int interval, bool debug)
	: _socket(io)
	, _timer(io)
	, _interval(interval)
	, _debug(debug)
{
	loadSensorMapping();

	try {
		_socket.listen(boost::asio::ip::address_v6::from_string("::"));
	} catch ( const hexabus::NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
		exit(1);
	}

	_socket.onPacketReceived(boost::bind(&HexabusServer::epqueryhandler, this, _1, _2), hf::isEndpointQuery());
	_socket.onPacketReceived(boost::bind(&HexabusServer::eid0handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_DEVICE_DESCRIPTOR);
	_socket.onPacketReceived(boost::bind(&HexabusServer::eid2handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_POWER_METER);
	//_socket.onPacketReceived(boost::bind(&HexabusServer::eid21handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_GEN_POWER_METER);
	_socket.onPacketReceived(boost::bind(&HexabusServer::l1handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_FLUKSO_L1);
	_socket.onPacketReceived(boost::bind(&HexabusServer::l2handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_FLUKSO_L2);
	_socket.onPacketReceived(boost::bind(&HexabusServer::l3handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_FLUKSO_L3);
	_socket.onPacketReceived(boost::bind(&HexabusServer::s01handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_FLUKSO_S01);
	_socket.onPacketReceived(boost::bind(&HexabusServer::s02handler, this, _1, _2), hf::isQuery() && hf::eid() == EP_FLUKSO_S02);

	_socket.onAsyncError(boost::bind(&HexabusServer::errorhandler, this, _1));
	broadcast_handler(boost::system::error_code());
}

void HexabusServer::epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const hexabus::EndpointQueryPacket* packet = dynamic_cast<const hexabus::EndpointQueryPacket*>(&p);
	try {
		switch (packet->eid()) {
			case EP_DEVICE_DESCRIPTOR:
				_socket.send(hexabus::EndpointInfoPacket(EP_DEVICE_DESCRIPTOR, HXB_DTYPE_UINT32, "HexaDaemon", HXB_FLAG_NONE), from);
				break;
			case EP_POWER_METER:
				_socket.send(hexabus::EndpointInfoPacket(EP_POWER_METER, HXB_DTYPE_UINT32, "HexabusPlug+ Power meter (W)", HXB_FLAG_NONE), from);
				break;
			case EP_GEN_POWER_METER:
				_socket.send(hexabus::EndpointInfoPacket(EP_GEN_POWER_METER, HXB_DTYPE_66BYTES, "Power meter (W)", HXB_FLAG_NONE), from);
				break;
			case EP_FLUKSO_L1:
				_socket.send(hexabus::EndpointInfoPacket(EP_FLUKSO_L1, HXB_DTYPE_UINT32, "Flukso Phase 1", HXB_FLAG_NONE), from);
				break;
			case EP_FLUKSO_L2:
				_socket.send(hexabus::EndpointInfoPacket(EP_FLUKSO_L2, HXB_DTYPE_UINT32, "Flukso Phase 2", HXB_FLAG_NONE), from);
				break;
			case EP_FLUKSO_L3:
				_socket.send(hexabus::EndpointInfoPacket(EP_FLUKSO_L3, HXB_DTYPE_UINT32, "Flukso Phase 3", HXB_FLAG_NONE), from);
				break;
			case EP_FLUKSO_S01:
				_socket.send(hexabus::EndpointInfoPacket(EP_FLUKSO_S01, HXB_DTYPE_UINT32, "Flukso S0 1", HXB_FLAG_NONE), from);
				break;
			case EP_FLUKSO_S02:
				_socket.send(hexabus::EndpointInfoPacket(EP_FLUKSO_S02, HXB_DTYPE_UINT32, "Flukso S0 2", HXB_FLAG_NONE), from);
				break;
		}
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for EID 0 received" << std::endl;
	try {
		uint32_t eids = (1 << EP_DEVICE_DESCRIPTOR) | (1 << EP_POWER_METER) | (1 << EP_GEN_POWER_METER);
		_socket.send(hexabus::InfoPacket<uint32_t>(EP_DEVICE_DESCRIPTOR, eids), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for EID 2 received" << std::endl;
	try {
		int value = getFluksoValue();
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_POWER_METER, value), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

/*void HexabusServer::eid21handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for EID 21 received" << std::endl;

	updateFluksoValues();

	try {
		for ( std::map<std::string, uint32_t>::iterator it = _flukso_values.begin(); it != _flukso_values.end(); it++ )
		{
			boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> data;
			unsigned short hash;
			for ( unsigned int pos = 0; pos < 16; pos++ )
			{
				std::stringstream ss(it->first.substr(2*pos, 2));
				ss >> std::hex >> hash;
				data[pos] = hash;
			}
			union {
				uint32_t u32;
				char raw[sizeof(it->second)];
			} c = { htonl(it->second) };
			for ( unsigned int pos = 0; pos < sizeof(uint32_t); pos++ )
			{
				data[16+pos] = c.raw[pos];
			}

			for ( unsigned int pos = 16 + sizeof(uint32_t); pos < HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH; pos++ )
				data[pos] = 0;

			_socket.send(hexabus::InfoPacket< boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >(EP_GEN_POWER_METER, data), from);
		}
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}*/

void HexabusServer::l1handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for Phase 1 received" << std::endl;
	try {
		int value = _flukso_values[_sensor_mapping[1]];
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_FLUKSO_L1, value), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::l2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for Phase 2 received" << std::endl;
	try {
		int value = _flukso_values[_sensor_mapping[2]];
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_FLUKSO_L2, value), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::l3handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for Phase 3 received" << std::endl;
	try {
		int value = _flukso_values[_sensor_mapping[3]];
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_FLUKSO_L3, value), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::s01handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for S0 1 received" << std::endl;
	try {
		int value = _flukso_values[_sensor_mapping[4]];
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_FLUKSO_S01, value), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::s02handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	_debug && std::cout << "Query for S0 2 received" << std::endl;
	try {
		int value = _flukso_values[_sensor_mapping[5]];
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_FLUKSO_S02, value), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::broadcast_handler(const boost::system::error_code& error)
{
	_debug && std::cout << "Broadcasting current values." << std::endl;
	if ( !error )
	{
		int value = getFluksoValue();
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(EP_POWER_METER, value));

		_timer.expires_from_now(boost::posix_time::seconds(_interval+(rand()%_interval)));
		_timer.async_wait(boost::bind(&HexabusServer::broadcast_handler, this, _1));
	} else {
		std::cerr << "Error occured in deadline_timer: " << error.message() << "(" << error.value() << ")." << std::endl;
	}
}

void HexabusServer::errorhandler(const hexabus::GenericException& error)
{
	const hexabus::NetworkException* nerror;
	if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&error))) {
		std::cerr << "Error while receiving hexabus packets: " << nerror->code().message() << std::endl;
	} else {
		std::cerr << "Error while receiving hexabus packets: " << error.what() << std::endl;
	}
}

int HexabusServer::getFluksoValue()
{
	updateFluksoValues();
	int result = 0;

	for ( std::map<std::string, uint32_t>::iterator it = _flukso_values.begin(); it != _flukso_values.end(); it++ )
		result += it->second;

	return result;
}

void HexabusServer::updateFluksoValues()
{
	bf::path p("/var/run/fluksod/sensor/");

	if ( exists(p) && is_directory(p) )
	{
		for ( bf::directory_iterator sensors(p); sensors != bf::directory_iterator(); sensors++ )
		{
			std::string filename = (*sensors).path().filename().string();
			_debug && std::cout << "Parsing file: " << filename << std::endl;

			//convert hash from hex to binary
			boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> data;
			unsigned short hash;
			for ( unsigned int pos = 0; pos < 16; pos++ )
			{
				std::stringstream ss(filename.substr(2*pos, 2));
				ss >> std::hex >> hash;
				data[pos] = hash;
			}

			std::ifstream file;
			file.open((*sensors).path().string().c_str());
			if ( file.fail() )
				continue;

			std::string flukso_data;
			file >> flukso_data;
			file.close();
			//extract last value != "nan" from the json array
			boost::regex r("^\\[(?:\\[[[:digit:]]*,[[:digit:]]*\\],)*\\[[[:digit:]]*,([[:digit:]]*)\\](?:,\\[[[:digit:]]*,\"nan\"\\])*\\]$");
			boost::match_results<std::string::const_iterator> what;

			uint32_t value = 0;
			if ( boost::regex_search(flukso_data, what, r))
				value = boost::lexical_cast<uint32_t>(std::string(what[1].first, what[1].second));

			_flukso_values[filename] = value;
			_debug && std::cout << "Updating _flukso_values[" << filename << "] = " << value << std::endl;

			union {
				uint32_t u32;
				char raw[sizeof(value)];
			} c = { htonl(value) };
			for ( unsigned int pos = 0; pos < sizeof(value); pos++ )
			{
				data[16+pos] = c.raw[pos];
			}

			//pad array with zeros
			for ( unsigned int pos = 16 + sizeof(value); pos < HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH; pos++ )
				data[pos] = 0;

			_socket.send(hexabus::InfoPacket< boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >(EP_GEN_POWER_METER, data));
		}
	}
}

void HexabusServer::loadSensorMapping()
{
	uci_context *ctx = uci_alloc_context();
	uci_package *flukso;
	uci_load(ctx, "flukso", &flukso);
	for ( unsigned int i = 1; i < 6; i++ )
	{
		int port;
		std::string id;
		uci_ptr res;
		char portKey[13], idKey[11];
		snprintf(portKey, 13, "flukso.%d.port", i);
		snprintf(idKey, 11, "flukso.%d.id", i);
		uci_lookup_ptr(ctx, &res, portKey, false);
		if (!(res.flags & uci_ptr::UCI_LOOKUP_COMPLETE)) {
			std::cerr << "Unable to load sensor port configuration" << std::endl;
			return;
		}
		if (res.last->type != UCI_TYPE_OPTION) {
			std::cerr << "Looking up sensor port configuration failed" << std::endl;
			return;
		}
		if (res.o->type != UCI_TYPE_STRING) {
			std::cerr << "Looking up sensor port configuration failed" << std::endl;
			return;
		}
		_debug && std::cout << "uci load: " << portKey << ": " << res.o->v.string << std::endl;
		port = atoi(res.o->v.string);
		uci_lookup_ptr(ctx, &res, idKey, false);
		if (!(res.flags & uci_ptr::UCI_LOOKUP_COMPLETE)) {
			std::cerr << "Unable to load sensor id configuration" << std::endl;
			return;
		}
		if (res.last->type != UCI_TYPE_OPTION) {
			std::cerr << "Looking up sensor id configuration failed" << std::endl;
			return;
		}
		if (res.o->type != UCI_TYPE_STRING) {
			std::cerr << "Looking up sensor id configuration failed" << std::endl;
			return;
		}
		_debug && std::cout << "uci load: " << idKey << ": " << res.o->v.string << std::endl;
		id = res.o->v.string;
		_sensor_mapping[port] = id;
	}
	uci_free_context(ctx);
}
