# HTTP listeners and SSDP-only devices

`UpnpInit2(interface, port)` preserves the traditional SDK startup: when the
internal webserver is compiled in, initialization opens HTTP TCP listeners before
any device or client is registered. Depending on the build and interface, these
serve IPv4, IPv6 link-local and IPv6 unique/global addresses. The listeners are
shared by HTTP content serving, SOAP control and GENA eventing.

`UpnpInit3(interface, port, UPNP_INIT_DEFAULT)` has the same behavior.
`UpnpInit3(interface, port, UPNP_INIT_NO_WEB_SERVER)` initializes the SDK without
opening any of those TCP listeners. SSDP discovery and advertisements still use
UDP sockets, including the SDK's local control socket. Outgoing HTTP requests,
such as fetching a device description from another webserver, remain available.
This option does not make the process socket-free.

## Announce a BasicDevice using an existing webserver

A BasicDevice without services needs neither SOAP control nor GENA eventing.
Its description can be served by an application's existing HTTP server. There
is no reason to expose another HTTP listener solely to announce that device.

Publish a description such as the following on the existing server:

```xml
<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
    <friendlyName>Example BasicDevice</friendlyName>
    <manufacturer>Example</manufacturer>
    <modelName>BasicDevice</modelName>
    <UDN>uuid:9b46b0a8-98e6-40aa-a12a-5a0b1baacafd</UDN>
  </device>
</root>
```

Choose a stable, unique UDN for the actual device. The URL passed to registration
must be reachable both by the SDK (which downloads the XML) and by discovering
control points. The following sequence illustrates an IPv4 device; `callback`
is the application's `Upnp_FunPtr` callback, which is required even for a device
without services:

```c
UpnpDevice_Handle device;
int ret = UpnpInit3("eth0", 0, UPNP_INIT_NO_WEB_SERVER);
if (ret != UPNP_E_SUCCESS)
    return ret;

ret = UpnpRegisterRootDevice3(
    "http://192.0.2.10:8080/device.xml", callback, NULL, &device, AF_INET);
if (ret == UPNP_E_SUCCESS) {
    ret = UpnpSendAdvertisement(device, 1800);
    if (ret == UPNP_E_SUCCESS) {
        /* Run the application until it is time to shut down. */
    }
    UpnpUnRegisterRootDevice(device);
}
UpnpFinish();
return ret;
```

The advertised SSDP `LOCATION` remains the supplied external URL. Registration,
advertisements, client registration and server-port queries never implicitly
activate the internal HTTP listeners. Registration modes that require the SDK
to host an XML alias (including local file/buffer descriptions) instead fail with
`UPNP_E_NO_WEB_SERVER` while listeners are disabled.

## Explicit activation and shutdown

After successful initialization, `UpnpEnableWebserver(1)` opens the listeners and
enables HTTP content serving. The preferred port from initialization is retained
for this purpose; the normal port-selection rules apply, so a conflict can result
in another free port. `UpnpGetServerPort()`, `UpnpGetServerPort6()` and
`UpnpGetServerUlaGuaPort6()` report actual ports, or `0` for closed/unavailable
listeners. Querying them has no side effects. Settings can be configured before
activation.

`UpnpEnableWebserver(0)` closes all SDK HTTP listeners and shuts down the accepted
connections, including SOAP and GENA connections. Running application callbacks
may finish. The call does not wait for those callbacks and can itself be made
from a callback. Internal content/settings resources remain allocated until
`UpnpFinish()` has joined the workers, so requests already in progress can unwind
safely. Repeated calls with the current state succeed without changing it.

**Compatibility change:** older versions only disabled content serving and left
the HTTP listeners, SOAP and GENA handling active. Applications that relied on
that behavior must keep the server enabled when they need those endpoints.

SSDP continues running after HTTP shutdown, and device/client registrations are
retained. Shutting down HTTP does not unregister devices, rewrite descriptions,
withdraw advertised service URLs or automatically cancel/recreate subscriptions.
Applications must manage those actions according to their advertised services;
a BasicDevice with externally served XML and no services requires none of the
HTTP endpoints. New synchronous/asynchronous GENA subscriptions are rejected
with `UPNP_E_NO_WEB_SERVER` while disabled. After re-enabling HTTP, recreate any
required event subscriptions with the actual callback ports.

`UpnpIsWebserverEnabled()` reports the listener/content-serving state rather than
the SSDP state. `UpnpEnableWebserver()` requires an initialized SDK; it is not a
configuration API to call before initialization. When internal webserver support
is compiled out, initialization never opens HTTP listeners and enabling/disabling
returns `UPNP_E_NO_WEB_SERVER`.
