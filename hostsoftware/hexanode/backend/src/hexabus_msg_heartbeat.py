#!/usr/bin/python3

import json
import argparse
import hashlib
import hmac
import http.client
import ssl
import urllib.parse
import base64
import io
import tempfile
import sys
import traceback
import subprocess

global device_id
global signing_key
global api_url

def sign_message(message):
	mac = hmac.new(bytes(signing_key, 'ascii'), digestmod=hashlib.sha1)
	mac.update(message)
	return mac.hexdigest()

def http_request(method, url, body):
	message = bytes(json.dumps(body), 'utf-8')
	signature = sign_message(message)

	url_parts = urllib.parse.urlparse(url, allow_fragments=False)
	context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
	context.verify_mode = ssl.CERT_NONE
	client = http.client.HTTPSConnection(url_parts.hostname, url_parts.port, context=context)
	client.request(method, url_parts.path, message, {
		'X-Digest': signature,
		'X-Version': '1.0',
		'Accept': 'application/json,text/html',
		'Content-Type': 'application/json',
		'User-Agent': 'hexabus_msg_heartbeat'
	})
	response = client.getresponse()
	if response.status != 200:
		print("Request failed with HTTP status {0}".format(response.status))
		exit(2)
	return json.loads(response.read().decode())


def perform_heartbeat():
	message = {
		'firmware': {
			'version': '0.6.0-1',
			'releasetime': '20120524_1158',
			'build': 'f0ba69e4fea1d0c411a068e5a19d0734511805bd',
			'tag': 'flukso-2.0.3-rc1-19-gf0ba69e'
		}
	}
	return http_request("POST", api_url + "/device/" + device_id, message)

def download_upgrade_package(target):
	content = base64.decodebytes(bytes(http_request("GET", api_url + "/firmware/" + device_id, {})["data"], 'ascii'))
	print(content)
	target.write(content)
	target.flush()

def perform_upgrade():
	with tempfile.NamedTemporaryFile(prefix="update_", suffix=".sh") as target_file:
		download_upgrade_package(target_file)
		success = subprocess.call(["/bin/sh", target_file.name]) == 0
		if success:
			http_request("POST", api_url + "/event/105", { 'device': device_id })
		else:
			http_request("POST", api_url + "/event/106", { 'device': device_id })

def perform_support(config):
	with tempfile.NamedTemporaryFile(prefix="device_key_") as device_key:
		device_key.write(base64.decodebytes(bytes(config["devicekey"], "ascii")))
		device_key.flush()
		with open("/root/.ssh/authorized_keys", "a") as authorized_keys:
			authorized_keys.write(config["techkey"])
		with open("/root/.ssh/known_hosts", "a") as known_hosts:
			known_hosts.write(config["host"] + " " + " ".join(config["hostkey"].split(" ")[0:2]) + "\n")
		cmd = [
				"dbclient",
				"-i", device_key.name,
				"-p", str(config["port"]),
				"-l", config["user"],
				"-N",
				"-R", "{0}:localhost:22".format(config["tunnelPort"]),
				config["host"]]
		subprocess.call(cmd)

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--url")
	parser.add_argument("--device")
	parser.add_argument("--key")
	args = parser.parse_args()
	global device_id
	device_id = args.device
	global signing_key
	signing_key = args.key
	global api_url
	api_url = args.url
	try:
		status = perform_heartbeat()
		if status["upgrade"] != 0:
			perform_upgrade()
		if "support" in status != None:
			perform_support(status["support"])

	except Exception as e:
		print("Heartbeat failed:")
		print(e)
		traceback.print_tb(sys.exc_info()[2])
		exit(10)
