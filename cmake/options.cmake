function (UPNP_deprecated_option old_name new_name description default)
	if (DEFINED ${old_name})
		message (DEPRECATION "${old_name} is a deprecated option and will be ignored after 1.14.x + 2. Use ${new_name} instead")
		set (default ${${old_name}})
	endif()

	option (${new_name} ${description} ${default})
endfunction()

option (BUILD_TESTING "Run Tests after compile" ON)
UPNP_deprecated_option (scriptsupport  IXML_ENABLE_SCRIPT_SUPPORT "script support for IXML document tree, see ixml.h" ON)
option (UPNP_BUILD_SHARED "Build shared libraries" ON)
option (UPNP_BUILD_STATIC "Build static libraries" ON)
UPNP_deprecated_option (samples UPNP_BUILD_SAMPLES "compilation of upnp/sample/ code" ON)
UPNP_deprecated_option (blocking_tcp_connections UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS "blocking TCP connections" OFF)
UPNP_deprecated_option (client UPNP_ENABLE_CLIENT_API "control point code (client)" ON)
UPNP_deprecated_option (device UPNP_ENABLE_DEVICE_API "device specific code (implies --disable-webserver if disabled)" ON)
UPNP_deprecated_option (gena UPNP_ENABLE_GENA "GENA part" ON)
UPNP_deprecated_option (tools UPNP_ENABLE_HELPER_API_TOOLS "helper APIs in upnptools.h" ON)
UPNP_deprecated_option (ipv6 UPNP_ENABLE_IPV6 "ipv6 support" ON)
UPNP_deprecated_option (optssdp UPNP_ENABLE_OPTIONAL_SSDP_HEADERS "optional SSDP headers support" ON)
UPNP_deprecated_option (open_ssl UPNP_ENABLE_OPEN_SSL "open-ssl support" OFF)
UPNP_deprecated_option (soap UPNP_ENABLE_SOAP "SOAP part" ON)
UPNP_deprecated_option (ssdp UPNP_ENABLE_SSDP "SSDP part" ON)
UPNP_deprecated_option (unspecified_server UPNP_ENABLE_UNSPECIFIED_SERVER "unspecified SERVER header" OFF)
UPNP_deprecated_option (webserver UPNP_ENABLE_WEBSERVER "integrated web server" ${UPNP_ENABLE_DEVICE_API})
UPNP_deprecated_option (reuseaddr UPNP_MINISERVER_REUSEADDR "Bind the miniserver socket with SO_REUSEADDR to allow clean restarts" ON)

if (UPNP_ENABLE_WEBSERVER AND NOT UPNP_ENABLE_DEVICE_API)
	message (FATAL_ERROR "The webserver does not work without the device-api code")
endif()

set (IXML_HAVE_SCRIPTSUPPORT ${IXML_ENABLE_SCRIPT_SUPPORT}) #see ixml.h
set (UPNP_HAVE_CLIENT ${UPNP_ENABLE_CLIENT_API}) #see upnpconfig.h
set (UPNP_HAVE_DEVICE ${UPNP_ENABLE_DEVICE_API}) #see upnpconfig.h
set (UPNP_HAVE_GENA ${UPNP_ENABLE_GENA}) #see upnpconfig.h
set (UPNP_HAVE_OPTSSDP ${UPNP_ENABLE_OPTIONAL_SSDP_HEADERS}) #see upnpconfig.h
set (UPNP_HAVE_SOAP ${UPNP_ENABLE_SOAP}) #see upnpconfig.h
set (UPNP_HAVE_SSDP ${UPNP_ENABLE_SSDP}) #see upnpconfig.h
set (UPNP_HAVE_TOOLS ${UPNP_ENABLE_HELPER_API_TOOLS}) #see upnpconfig.h
set (UPNP_HAVE_WEBSERVER ${UPNP_ENABLE_WEBSERVER}) #see upnpconfig.h
