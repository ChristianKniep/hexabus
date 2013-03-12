#include <libhexanode/common.hpp>
#include <libhexanode/sensor.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <boost/network/uri/uri_io.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;
#include <iostream>
#include <sstream>

using namespace boost::network;

int main(int argc, char *argv[]) {
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexanode version and exit")
    ("frontendurl,u", po::value<std::string>(), "URL of frontend API")
    ;
  po::positional_options_description p;
  p.add("frontendurl", 1);
  po::variables_map vm;

  // Begin processing of commandline parameters.
  try {
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    hexanode::VersionInfo v;
    std::cout << "hexanode demo data generator" << std::endl;
    std::cout << "hexanode version " << v.get_version() << std::endl;
    return 0;
  }

  uri::uri base_uri;

  if (vm.count("frontendurl") != 1) {
    std::cerr << "No frontend URL given - exiting." << std::endl;
    return 1;
  } else {
    base_uri << uri::scheme("http") 
      << uri::host(vm["frontendurl"].as<std::string>())
      << uri::path("/api");
  }

  std::cout << "Using frontend url " << base_uri << std::endl;

  http::client client;
  std::vector<hexanode::Sensor::Ptr> sensors;
  uint16_t max_id = 4;
  double min_value = 15;
  double max_value = 30;

  boost::random::mt19937 gen;
  boost::random::uniform_int_distribution<> dist(min_value, max_value);
  for (uint16_t id=0; id<max_id; ++id) {
    std::ostringstream oss;
    oss << "Sensor_" << id;
    hexanode::Sensor::Ptr new_sensor(new hexanode::Sensor(oss.str(), oss.str(), min_value, max_value));
    sensors.push_back(new_sensor);
  }
  std::cout << "Will now push values." << std::endl;
  while(true) {
    try {
      for (uint16_t sid=0; sid < max_id; ++sid) {
        hexanode::Sensor::Ptr sensor=sensors.at(sid);
        sensor->post_value(client, base_uri, dist(gen));
      }
    } catch (const std::exception& e) {
      std::cout << "Cannot submit sensor values (" << e.what() << ")" << std::endl;
      std::cout << "Attempting to re-create sensors." << std::endl;
      for (uint16_t sid=0; sid < max_id; ++sid) {
        hexanode::Sensor::Ptr sensor=sensors[sid];
        sensor->put(client, base_uri, 0.0); 
      }
      continue; // do not sleep
    }
    boost::this_thread::sleep( boost::posix_time::seconds(3) );
  }
}
