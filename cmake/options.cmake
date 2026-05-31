set (testing_default ON)

if(DEFINED ENABLE_TESTING AND NOT IXML_ENABLE_TESTING AND NOT UPNP_ENABLE_TESTING)
	message (DEPRECATION "PUPNP now has IXML_ENABLE_TESTING and UPNP_ENABLE_TESTING options")
	set (testing_default §{BUILD_TESTING})
endif()

option (IXML_ENABLE_SCRIPT_SUPPORT "script support for IXML document tree, see ixml.h" ON)
option (IXML_ENABLE_TESTING "enable tests for ixml" ${testing_default})
option (IXML_ENABLE_TESTING_INTEGRATION "enable integrationtests for ixml" ${testing_default})

option (UPNP_BUILD_SAMPLES "compilation of upnp/sample/ code" ON)
option (UPNP_BUILD_SHARED "Build shared libraries" ON)
option (UPNP_BUILD_STATIC "Build static libraries" ON)
option (UPNP_ENABLE_BACKTRACE "Print backtrace on first thread pool overflow" ON)
option (UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS "blocking TCP connections" OFF)
option (UPNP_ENABLE_CLIENT_API "control point code (client)" ON)
option (UPNP_ENABLE_DEBUG "debug logging (UpnpInitLog/UpnpSetLogLevel/UpnpGetDebugFile)" OFF)
option (UPNP_ENABLE_DEVICE_API "device specific code (implies --disable-webserver if disabled)" ON)
option (UPNP_ENABLE_GENA "GENA part" ON)
option (UPNP_ENABLE_HELPER_API_TOOLS "helper APIs in upnptools.h" ON)
option (UPNP_ENABLE_IPV6 "ipv6 support" ON)
option (UPNP_ENABLE_OPEN_SSL "open-ssl support" OFF)
option (UPNP_ENABLE_OPTIONAL_SSDP_HEADERS "optional SSDP headers support" ON)
option (UPNP_ENABLE_SOAP "SOAP part" ON)
option (UPNP_ENABLE_SSDP "SSDP part" ON)
option (UPNP_ENABLE_TESTING "enable tests for upnp" ${testing_default})
option (UPNP_ENABLE_TESTING_INTEGRATION "enable integrationtests for upnp" ${testing_default})
option (UPNP_ENABLE_UNSPECIFIED_SERVER "unspecified SERVER header" OFF)
option (UPNP_ENABLE_WEBSERVER "integrated web server" ${UPNP_ENABLE_DEVICE_API})
option (UPNP_MINISERVER_REUSEADDR "Bind the miniserver socket with SO_REUSEADDR to allow clean restarts" ON)

if (UPNP_ENABLE_WEBSERVER AND NOT UPNP_ENABLE_DEVICE_API)
	message (FATAL_ERROR "The webserver does not work without the device-api code")
endif()

set (IXML_HAVE_SCRIPTSUPPORT ${IXML_ENABLE_SCRIPT_SUPPORT}) #see ixml.h
set (UPNP_HAVE_CLIENT ${UPNP_ENABLE_CLIENT_API}) #see upnpconfig.h
set (UPNP_HAVE_DEBUG ${UPNP_ENABLE_DEBUG}) #see upnpconfig.h
set (UPNP_HAVE_DEVICE ${UPNP_ENABLE_DEVICE_API}) #see upnpconfig.h
set (UPNP_HAVE_GENA ${UPNP_ENABLE_GENA}) #see upnpconfig.h
set (UPNP_HAVE_OPTSSDP ${UPNP_ENABLE_OPTIONAL_SSDP_HEADERS}) #see upnpconfig.h
set (UPNP_HAVE_SOAP ${UPNP_ENABLE_SOAP}) #see upnpconfig.h
set (UPNP_HAVE_SSDP ${UPNP_ENABLE_SSDP}) #see upnpconfig.h
set (UPNP_HAVE_TOOLS ${UPNP_ENABLE_HELPER_API_TOOLS}) #see upnpconfig.h
set (UPNP_HAVE_WEBSERVER ${UPNP_ENABLE_WEBSERVER}) #see upnpconfig.h
