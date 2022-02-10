include (CMakeDependentOption)

option (UPNP_BUILD_SHARED "Build shared libraries" ON)
option (UPNP_BUILD_STATIC "Build static libraries" ON)
option (BUILD_TESTING "Run Tests after compile" ON)
option (client "control point code (client)" ON)
option (device "device specific code (implies --disable-webserver if disabled)" ON)
cmake_dependent_option (webserver "integrated web server" ON NOT device OFF)
option (optssdp "optional SSDP headers support" ON)
option (tools "helper APIs in upnptools.h" ON)
option (ipv6 "ipv6 support" ON)
option (unspecified_server "unspecified SERVER header" OFF)
option (open_ssl "open-ssl support" OFF)
option (blocking_tcp_connections "blocking TCP connections" OFF)
option (scriptsupport "script support for IXML document tree, see ixml.h" ON)
option (reuseaddr "Bind the miniserver socket with SO_REUSEADDR to allow clean restarts" ON)
option (samples "compilation of upnp/sample/ code" ON)
option (soap "SOAP part" ON)
option (ssdp "SSDP part" ON)
option (gena "GENA part" ON)

if (client)
	set (UPNP_HAVE_CLIENT 1) #see upnpconfig.h
endif()

if (device)
	set (UPNP_HAVE_DEVICE 1) #see upnpconfig.h
endif()

set (UPNP_HAVE_WEBSERVER ${webserver}) #see upnpconfig.h

if (ssdp)
	set (UPNP_HAVE_SSDP 1) #see upnpconfig.h
endif()

if (optssdp)
	set (UPNP_HAVE_OPTSSDP 1) #see upnpconfig.h
endif()

if (soap)
	set (UPNP_HAVE_SOAP 1) #see upnpconfig.h
endif()

if (gena)
	set (UPNP_HAVE_GENA 1) #see upnpconfig.h
endif()

if (gena OR optssdp)
	set (uuid TRUE)
endif()

if (tools)
	set (UPNP_HAVE_TOOLS 1) #see upnpconfig.h
endif()

if (ipv6)
	set (UPNP_ENABLE_IPV6 1) #see upnpconfig.h
endif()

set (UPNP_ENABLE_UNSPECIFIED_SERVER ${unspecified_server}) #see upnpconfig.h

if (blocking_tcp_connections)
	set (UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS 1) #see upnpconfig.h
endif()

set (IXML_HAVE_SCRIPTSUPPORT ${scriptsupport}) #see upnpconfig.h
set (UPNP_MINISERVER_REUSEADDR ${reuseaddr}) #see upnpconfig.h
