var nconf=require('nconf');
var exec = require('child_process').exec;

var Wizard = function() {

	expand_ipv6_address = function(address) {
		if(address.indexOf("::") != -1) {
			var sides = address.split("::");
			var missing_groups = 8 - sides[0].split(":").length - sides[1].split(":").length;
			var expansion = ":";
			for(var i=0; i<missing_groups; i++) {
				expansion += "0000:";
			}
			address = sides[0] + expansion + sides[1];
		}
		var groups = address.split(":");
		var expanded_address = "";
		for(var i=0; i<8; i++) {
			while(groups[i].length < 4) {
				groups[i] = "0"+groups[i];
			}
		}
		return(groups.join(":"));
	}

	get_ip_addresses = function() {
		var os=require('os');
		var ifaces=os.networkInterfaces();
		var ips={};
		for (var dev in ifaces) {
			if(dev!='lo'){
				ips[dev]={}
				var i=0;
				ifaces[dev].forEach(function(details){
					if (details.family=='IPv4'||details.family=='IPv6') {
						ips[dev][i]=details.address;
						i++;
					}
				});
			}
		}
		console.log(ips);
		return ips
	}

	this.get_devices = function(callback) {
		// Get devices from external tool, e.g. hexinfo
		exec('hexinfo -cj -d - -I usb0', function(error, stdout, stderr){
		//exec('cat tools/devices.json', function(error, stdout, stderr){
			if(error!==null) {
				callback(JSON.parse("{}"));
			} else {
				callback(JSON.parse(stdout));
			}
		});
	}

	this.get_status = function(req, res) {
		this.is_connected(function(connected) {
			var ip_addresses=get_ip_addresses();
			res.render('status.ejs',
			{
				"server": nconf.get('server'), 
				"port": nconf.get('port'),
				"connected": connected,
				"ip_addresses": ip_addresses,
				"devicelist": ''
			});
		});
	}

	this.show_wizard = function(req, res) {
		var ip_addresses=get_ip_addresses();
		res.render('wizard.ejs',
			{
				"server": nconf.get('server'), 
				"port": nconf.get('port'),
				"ip_addresses": ip_addresses,
				"devicelist": ''
			});
	}

	this.is_connected = function(callback) {
		exec('sudo hxb-net-autoconf', function(error, stdout, stderr) {
			if(error !== null) {
				callback(false);
			} else {
				// TODO use heartbeat
				require('dns').resolve('mysmartgrid.de', function(error) {
					if(error) {
						is_connected=false;
					} else {
						is_connected=true;
					}
				callback(stdout);
			});
			}
		});
	}

	this.get_activation = function(callback) {
		// execute hexabus_msg_bridge -A
		exec('hexabus_msg_bridge -A', function(error, stdout, stderr) {
			if(error !== null) {
				callback("Activation code could not be generated");
			} else {
				callback(stdout);
			}
			});
	}

	this.submit_devices = function(devices) {
		for(var ip in devices) {
			var name=devices[ip];
			ip = expand_ipv6_address(ip);
			var fs = require('fs');
			var text = "target "+ip+";\ndevice_name "+name+";\nmachine 0;";
			fs.writeFile("/tmp/"+ip+".hba", text, function(err) {
				if(err) {
					console.log(err);
				} else {
					// execute: habsm -i /tmp/tmp.hba -o /tmp/tmp.hbs
					exec('hbasm -i /tmp/'+ip+'.hba -o /tmp/'+ip+'.hbs -d /usr/share/hexabus/std_datatypes.hb', function(error, stdout, stderr){
						// execute: hexaupload -p /tmp/tmp.hbs
						exec('hexaupload -p /tmp/'+ip+'.hbs && rm /tmp/'+ip+'.hbs', function(error, stdout, stderr){
							console.log(stdout);
						});
					});
				}
			});
		}
	}
	
}

module.exports = Wizard;
