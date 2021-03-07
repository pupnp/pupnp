# The Portable SDK for UPnP\* Devices (libupnp) <!-- omit in toc -->

- [The GitHub project page](https://github.com/pupnp/pupnp) is where all the real action happens.
- [The old Source Forge project page is linked here.](https://sourceforge.net/projects/pupnp)

| branch        | status                                                                                              |
| ------------- | --------------------------------------------------------------------------------------------------- |
| master        | ![master](https://github.com/pupnp/pupnp/workflows/Build/badge.svg)                                 |
| branch-1.14.x | ![1.14.x](https://github.com/pupnp/pupnp/workflows/Build/badge.svg?branch=branch-1.14.x)            |
| branch-1.12.x | ![1.12.x](https://github.com/pupnp/pupnp/workflows/C%2FC%2B%2B%20CI/badge.svg?branch=branch-1.12.x) |
| branch-1.10.x | ![1.10.x](https://github.com/pupnp/pupnp/workflows/C%2FC%2B%2B%20CI/badge.svg?branch=branch-1.10.x) |
| branch-1.8.x  | ![1.8.x](https://github.com/pupnp/pupnp/workflows/C%2FC%2B%2B%20CI/badge.svg?branch=branch-1.8.x)   |
| branch-1.6.x  | ![1.6.x](https://github.com/pupnp/pupnp/workflows/C%2FC%2B%2B%20CI/badge.svg?branch=branch-1.6.x)   |
| branch-1.4.x  | ![1.4.x](https://github.com/pupnp/pupnp/workflows/C%2FC%2B%2B%20CI/badge.svg?branch=branch-1.4.x)   |

Copyright (c) 2000-2003 Intel Corporation - All Rights Reserved.

Copyright (c) 2005-2006 Rémi Turboult <r3mi@users.sourceforge.net>

Copyright (c) 2006 Michel Pfeiffer and others <virtual_worlds@gmx.de>

See [LICENSE](site/LICENSE) for details.

## Table of Contents <!-- omit in toc -->

- [1. Overview](#1-overview)
- [2. General Information](#2-general-information)
- [3. Changelog](#3-changelog)
- [4. Documentation](#4-documentation)
- [5. Other projects that are using the SDK](#5-other-projects-that-are-using-the-sdk)
- [6. License Conditions](#6-license-conditions)
- [7. Release List](#7-release-list)
- [8. Package Contents](#8-package-contents)
- [9. System Requirements](#9-system-requirements)
- [10. Build Instructions](#10-build-instructions)
  - [10.1. Pre-requisites](#101-pre-requisites)
  - [10.2. Core Libraries](#102-core-libraries)
  - [10.3. Cross Compilation](#103-cross-compilation)
  - [10.4. Samples](#104-samples)
  - [10.5. Solaris Build](#105-solaris-build)
  - [10.6. Windows Build](#106-windows-build)
  - [10.7. CMake Build](#107-cmake-build)
- [11. Install/Uninstall Instructions](#11-installuninstall-instructions)
  - [11.1. Install](#111-install)
  - [11.2. Uninstall](#112-uninstall)
- [12. Product Release Notes](#12-product-release-notes)
- [13. New Features](#13-new-features)
- [14. Support and Contact Information](#14-support-and-contact-information)
- [15. IXML support for scripting languages](#15-ixml-support-for-scripting-languages)
- [16. Thanks](#16-thanks)

## 1. Overview

The Portable SDK for UPnP&trade; Devices is an SDK for development of UPnP device and control point applications. It consists of the core UPnP protocols along with a UPnP-specific eXtensible Markup Language (XML) parser supporting the Document Object Model (DOM) Level 2 API and an optional, integrated mini web server for serving UPnP related documents. It provides developers with an API and open source code for building control points, devices, and bridges that are compliant with Version 1.0 of the [Universal Plug and Play Device Architecture Specification](http://www.upnp.org/resources/upnpresources.zip) and supports several operating systems like Linux, *BSD, Solaris and others.

## 2. General Information

UPnP&trade; is an architecture that enables discovery, event notification, and control of devices on a network, independent of operating system, programming language, or physical network connection.&nbsp; UPnP&trade; is based on common Internet standards and specifications such as TCP/IP, HTTP, and XML. For detailed information about UPnP&trade;, including the UPnP&trade; Device Architecture Specification, please visit the [UPnP&trade; Forum web site](http://www.upnp.org/).

In 2000, Intel created the first version of the Linux SDK for UPnP&trade; Devices and subsequently released it to the open source community to foster growth of UPnP&trade;.&nbsp; To learn more about&nbsp; Intel's involvement with both UPnP&trade; and the SDK, please visit [Intel's Universal Plug and Play web site](http://www.intel.com/cd/ids/developer/asmo-na/eng/downloads/upnp/index.htm").

In 2006 this 100% compatible fork of the original project was created to
support further development. This way, the project now continues using the name "Portable UPnP&trade;" and as a project that is more open to contributions of the community. The main goal is the availability of the project for all important platforms to become a standard for UPnP&trade;.

## 3. [Changelog](ChangeLog)

## 4. Documentation

Documentation is available in PDF format from the [downloads](http://sourceforge.net/project/showfiles.php?group_id=166957) section. The documentation actually resides inside the source code itself and is built into the PDF file by an automated process using [Doxygen](https://www.doxygen.nl). Documentation for each function resides in a comment section immediately preceding the function.

## 5. Other projects that are using the SDK

This is a list of some of the projects and products hat are based on the SDK for UPnP&trade; Devices.&nbsp; Please let us know if you are working on a project and would like to see it listed here!

- [aMule](http://www.amule.org) has libupnp support to perform port forwarding.
- [Gerbera](https://gerbera.io) UPnP&trade; media server.
- [libmcupnp](http://sourceforge.net/projects/libmcupnp) is a Free UPnP(v1) library for easy "MediaServer:1 Client" implementations. The library is built on top of libupnp.
- [HD Network DVD Media Player](http://www.z500series.com)
- [eMule Morph](http://emulemorph.sourceforge.net) uses libupnp to forward ports automatically.
- [PeerStream Audio Video Server](http://www.peerstream.net).
- [GeeXboX uShare&trade;](http://ushare.geexbox.org) A/V media server.
- [MediaTomb](http://mediatomb.sourceforge.net) UPnP&trade; media server.
- The [Linux UPnP&trade; Internet Gateway Device](http://linux-igd.sourceforge.net) This project is a daemon that emulates Microsoft's Internet Connection Service (ICS). It implements the UPnP&trade; Internet Gateway Device (IGD) specification and allows UPnP&trade;-aware clients, such as MSN Messenger, to work properly from behind a NAT firewall.
- FreeBSD ports of both the [SDK for UPnP&trade; Devices](http://www.freebsd.org/cgi/cvsweb.cgi/ports/devel/upnp)"> and the [Linux UPnP&trade; IGD](http://www.freebsd.org/cgi/query-pr.cgi?pr=41295)" were contributed to the FreeBSD ports collection by [Yen-Ming Lee](http://www.leeym.com).
- [PseudoICSD](http://pseudoicsd.sf.net) is another daemon that provides UPnP&trade; Internet Gateway Device functionality on Linux systems.
- [IGD2 for linux](http://gitorious.org/igd2-for-linux) is an updated version of [Linux-IGD implementation](http://linux-igd.sourceforge.net>http://linux-igd.sourceforge.net). This new version is created on top of UPnP IGD:2 specifications available from [http://upnp.org/specs/gw/igd2](http://upnp.org/specs/gw/igd2). You can find more information about this project [here](http://gitorious.org/igd2-for-linux/pages/Home).

The links listed here point to external pages that are not under our control, that means we do not have any influence on their contents. Some jurisdictions have the point of view that the owner of a internet page is responsible for links to other webpages and the contents that can be found there. (Landgericht Hamburg, Judgement from 12th May 1998, 312 O 85/98).

Because of that, the operator of these pages is dissociating explicitly from all links made from here. If we are informed about pages with penal relevant names, links, contents or something else we'll remove links to these pages immediately in case that is technically possible and reasonable.

## 6. License Conditions

The Portable SDK for UPnP&trade; Devices is distributed under the BSD (Berkeley Standard Distribution) license. This license allows you to use the source code royalty free and does not place any restrictions on what you do with source code derived from the SDK. For full details on the license conditions, please consult the [LICENSE](site/LICENSE) file located inside the SDK distribution.

## 7. Release List

| Release Number | Date       | History                                  |
| -------------- | ---------- | ---------------------------------------- |
| 1.16.0         | TBA        | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.14.2         | 2021-02-28 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.14.1         | 2021-02-08 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.14.0         | 2020-07-20 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.12.1         | 2020-04-07 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.12.0         | 2020-01-22 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.10.1         | 2019-11-20 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.10.0         | 2019-11-01 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.7          | 2020-04-07 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.6          | 2019-11-20 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.5          | 2019-11-01 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.4          | 2018-10-25 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.3          | 2017-11-12 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.2          | 2017-11-12 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.1          | 2017-05-24 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.8.0          | 2017-01-04 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.25         | 2016-02-10 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.24         | 2017-11-19 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.23         | 2017-11-19 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.22         | 2017-05-30 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.21         | 2016-12-21 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.20         | 2016-07-07 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.19         | 2013-11-15 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.18         | 2013-01-29 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.17         | 2012-04-03 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.16         | 2012-03-21 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.15         | 2012-01-25 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.14         | 2011-11-17 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.13         | 2011-03-17 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.12         | 2011-02-08 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.11         | 2011-02-07 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.10         | 2011-01-30 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.9          | 2010-11-07 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.8          | 2010-10-21 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.7          | 2010-10-04 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.6          | 2008-04-25 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.5          | 2008-02-02 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.4          | 2008-01-26 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.3          | 2007-12-26 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.2          | 2007-12-10 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.1          | 2007-11-08 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.6.0          | 2007-06-23 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.6          | 2007-04-30 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.5          | 2007-04-28 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.4          | 2007-04-17 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.3          | 2007-03-06 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.2          | 2007-02-16 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.1          | 2006-08-11 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.4.0          | 2006-06-12 | [Portable UPnP SDK][Portable UPnP SDK]   |
| 1.3.1          | 2006-03-05 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.3.0          | 2006-03-04 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.2.1a         | 2003-11-08 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.2.1          | 2003-02-13 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.0.4          | 2001-08-15 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.0.3          | 2001-06-12 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.0.2          | 2001-02-07 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.0.1          | 2000-10-13 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 1.0.0          | 2000-08-31 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 0.9.1          | 2000-08-17 | [UPnP SDK for Linux][UPnP SDK for Linux] |
| 0.9.0          | 2000-08-01 | [UPnP SDK for Linux][UPnP SDK for Linux] |

[UPnP SDK for Linux]: https://sourceforge.net/projects/upnp/
[Portable UPnP SDK]: https://sourceforge.net/projects/pupnp/

## 8. Package Contents

The SDK for UPnP Devices contains the following:

| Path/File   | Description                                                                       |
| ----------- | --------------------------------------------------------------------------------- |
| README      | This file.  Contains the installation and build instructions.                     |
| LICENSE     | The licensing terms the SDK is distributed under.                                 |
| NEWS        | Changes and new features.                                                         |
| ixml/doc    | The files for generating the XML parser documentation from the source code.       |
| ixml/inc    | The public include files required to use the XML parser.                          |
| ixml/src    | The source code to the XML parser library.                                        |
| upnp/doc    | The files for generating the SDK documentation from the source code.              |
| upnp/inc    | The public include files required to use the SDK.                                 |
| upnp/src    | The source files comprising the SDK, libupnp.so.                                  |
| upnp/sample | A sample device and control point application, illustrating the usage of the SDK. |

## 9. System Requirements

The SDK for UPnP Devices is designed to compile and run under several operating systems.  It does, however, have dependencies on some packages that may not be installed by default.  All packages that it requires are listed below.

| Dependency | Description                                                                              |
| ---------- | ---------------------------------------------------------------------------------------- |
| libpthread | The header and library are installed as part of the glibc-devel package (or equivalent). |

Additionally, the documentation for the SDK can be auto-generated from the upnp.h header file using Doxygen, a documentation system for C, C++, IDL, and Java\*.  Doxygen generates the documentation in HTML or TeX format. Using some additional tools, the TeX output can be converted into a PDF file. To generate the documentation these tools are required:

| Package   | Description                                                                                                                                                                                                 |
| --------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Doxygen   | The homepage for Doxygen is <https://www.doxygen.nl/index.html>. The current version as of this release of the SDK is version 3.4.9. Doxygen is the only requirement for generating the HTML documentation. |
| LaTeX/TeX | To generate the PDF documentation, LaTeX and TeX tools are necessary. The tetex and tetex-latex packages provide these tools.                                                                               |
| dvips     | dvips converts the DVI file produced by LaTeX into a PostScript\* file. The tetex-dvips package provides this tool.                                                                                         |
| ps2pdf    | The final step to making the PDF is converting the PostStript\* into Portable Document Format. The ghostscript package provides this tool.                                                                  |

For the UPnP library to function correctly, networking must be configured properly for multicasting.  To do this:

```bash
% route add -net 239.0.0.0 netmask 255.0.0.0 eth0
```

where 'eth0' is the network adapter that the UPnP library will use.  Without this addition, device advertisements and control point searches will not function.

## 10. Build Instructions

### 10.1. Pre-requisites

Some packages/tools are required to build the library. Here's a minimal 'inspirational example'
that builds the library using a Docker Ubuntu image.

```bash
% docker run -it --rm ubuntu /bin/bash

% apt update \
  && apt install -y build-essential autoconf libtool pkg-config git shtool \
  && git clone http://github.com/pupnp/pupnp.git \
  && cd pupnp \
  && ./bootstrap

% ./configure
% make
```

### 10.2. Core Libraries

Note: On a git checkout, you need to run `./bootstrap` to generate the configure script.

```bash
% ./configure
% make
```

will build a version of the binaries without debug support, and with default options enabled (see below for options available at configure time).

```bash
% ./configure CFLAGS="-DSPARC_SOLARIS -mtune=<cputype> -mcpu=<cputype>"
% make
```

will build a Sparc Solaris version of the binaries without debug support and with default options enabled (see below for options available at configure time). Please note: \<cputype\> has to be replaced by a token that fits to your platform and CPU (e.g. "supersparc").

To build the documentation, assuming all the necessary tools are installed (see section 3):

To generate the HTML documentation:

```bash
% make html
```

To generate the PDF file:

```bash
% make pdf
```

A few options are available at configure time. Use "./configure --help" to display a complete list of options. Note that these options may be combined in any order. After installation, the file \<upnp/upnpconfig.h\> will provide a summary of the optional features that have been included in the library.

```bash
% ./configure --enable-debug
% make
```

will build a debug version with symbols support.

To build the library with the optional, integrated mini web server (note that this is the default):

```bash
% ./configure --enable-webserver
% make
```

To build without:

```bash
% ./configure --disable-webserver
% make
```

The SDK also contains some additional helper APIs, declared in inc/tools/upnptools.h.  If these additional tools are not required, they can be compiled out:

```bash
% ./configure --disable-tools
% make
```

By default, the tools are included in the library.

To further remove code that is not required, the library can be build with or with out the control point (client) or device specific code.  To remove this code:

```bash
% ./configure --disable-client
% make
```

to remove client only code or:

```bash
% ./configure --disable-device
% make
```

to remove device only code.

By default, both client and device code is included in the library. The integrated web server is automatically removed when configuring with --disable-device.

To build the library without large-file support (enabled by default):

```bash
% ./configure --disable-largefile
% make
```

To remove all the targets, object files, and built documentation:

```bash
% make clean
```

### 10.3. Cross Compilation

To cross compile the SDK, a special "configure" directive is all that is required:

```bash
% ./configure --host=arm-linux
% make
```

This will invoke the "arm-linux-gcc" cross compiler to build the library.

### 10.4. Samples

The SDK contains two samples: a TV device application and a control point that talks with the TV device.  They are found in the $(LIBUPNP)/upnp/sample directory.

To build the samples (note: this is the default behavior):

```bash
% ./configure --enable-samples
% make
```

will build the sample device "$(LIBUPNP)/upnp/tv_device" and sample control point "$(LIBUPNP)/upnp/tv_ctrlpt". Note : the sample device won't be built if --disable-device has been configured, and the sample control point won't be build if --disable-client has been configured.

To run the sample device, you need to create a tvdevice directory and move the web directory there, giving: "$(LIBUPNP)/upnp/sample/tvdevice/web". To run the sample invoke from the command line as follows:

```bash
% cd ./upnp/sample/tvdevice
% ../tv_device
```

### 10.5. Solaris Build

The building process for the Solaris operating system is similar to the one described above. Only the call to ./configure has to be done using an additional parameter:

```bash
% ./configure CFLAGS="-mcpu=<cputype> -mtune=<cputype> -DSPARC_SOLARIS"
```

where \<cputype\> has to be replaced by the appropriate CPU tuning flag (e.g. "supersparc"). Afterwards

```bash
% make
% make install
```

can be called as described above.

### 10.6. Windows Build

In order to build libupnp under Windows the pthreads-w32 package is required. You can download a self-extracting ZIP file from the following location:

<ftp://sources.redhat.com/pub/pthreads-win32/pthreads-w32-2-7-0-release.exe>

or possibly newer versions if available.

- Execute the self-extracting archive and copy the Pre-build.2 folder to the top level source folder.
- Rename Pre-build.2 to pthreads.
- Open the provided workspace build\libupnp.dsw with Visual C++ 6.0 and select Build->Build libupnp.dll (F7)
- In the build directory there are also VC8, VC9 and VC10 folders containing solution files for Visual Studio 2005/2008/2010 respectively.

If you use newer versions to build libupnp, eg Visual Studio 2003 or later, then you need to rebuild the pthreads package so it uses the same VC runtime as libupnp to prevent cross boundary runtime problems (see <http://msdn.microsoft.com/en-us/library/ms235460%28v=VS.100%29.aspx>). Just replace the files in the Pre-build.2 folder (renamed to pthreads as mentioned above) with the newly build versions. If you also use a newer version of pthreads-win32 then you should also replace the header files in that directory structure (obviously).

For building a static library instead of a DLL and for using the static pthreads-w32 library following switches need to be defined additionally:

UPNP_STATIC_LIB - for creating a statically linkable UPnP-library
PTW32_STATIC_LIB - for using the static pthreads32 library

### 10.7. CMake Build

In Order to build everything using the cmake build system, you just need to install cmake for your platform.
Standalone cmake is recommended, IDE's like Visual Studio have built-in support which works, but as cmake in general
encourages out-of-source builds and VS writes it's config into the source, cmake-gui should be used on windows.

All known options have the same meaning as stated in point 5.2. In Addition 2 options have been added.

- DOWNLOAD_AND_BUILD_DEP: This option is only available if a useable git program was found on your system.
  With this option on, the pthread4w package will be downloaded while configuring the build-env, then it will be build and installed along with upnp.

- BUILD_TESTING: This option activates the tests.

If you don't want to build pthreads4w in the same build as upnp, you can download it from <https://github.com/Vollstrecker/pthreads4w>.
Just build and install it. The libs and headers will be found, if you set CMAKE_INSTALL_PREFIX (the base install dir) to the same location.

For information on general usage of the cmake build system see: <https://cmake.org/cmake/help/v3.19/guide/user-interaction/index.html>

## 11. Install/Uninstall Instructions

### 11.1. Install

The top-level makefile for the UPnP SDK contains rules to install the necessary components.  To install the SDK, as root:

```bash
% make install
```

### 11.2. Uninstall

Likewise, the top-level makefile contains an uninstall rule, reversing the steps in the install:

```bash
% make uninstall
```

## 12. Product Release Notes

The SDK for UPnP Devices v1.2.1a has these known issues:

- The UPnP library may not work with older versions of gcc and libstdc++, causing a segmentation fault when the library loads.  It is recommended that gcc version 2.9 or later be used in building library.
- The UPnP library does not work the glibc 2.1.92-14 that ships with Red Hat 7.0.  For the library to function, you must updated the glibc and glibc-devel packages to 2.1.94-3 or later.  There is some issue with libpthreads that has been resolved in the 2.1.94 version.

## 13. New Features

See [ChangeLog file](https://github.com/pupnp/pupnp/blob/master/ChangeLog).

## 14. Support and Contact Information

Intel is not providing support for the SDK for UPnP Devices. Mailing lists and discussion boards can be found at <https://github.com/pupnp/pupnp/discussions>.

If you find this SDK useful, please send an email to upnp@intel.com and let us know.

\* Other brands, names, and trademarks are the property of their respective owners.

## 15. IXML support for scripting languages

The tree structure of XML documents created by IXML is hard to maintain when creating a binding for a scripting language. Even when many elements may never be used on the script side, it requires copying the entire tree structure once you start accessing elements several levels deep.Hence scriptsupport was added. To enable it compile while IXML_HAVE_SCRIPTSUPPORT has been defined (enabled by default). This allows control using only a list instead of a tree-like structure, and only nodes actually accessed need to be created instead of all the nodes in the tree.

Here's how its supposed to work:

- The scriptsupport allows you to add a callback when a node is freed on the C side, so appropriate action can be taken on the script side, see function ixmlSetBeforeFree().
- Instead of recreating the tree structure, an intermediate object should be created only for the nodes actually accessed. The object should be containing a pointer to the node and a 'valid flag' which is initially set to TRUE (the valid flag, can simply be the pointer to the node being NULL or not). Before creating the intermediate object, the custom tag 'ctag' can be used to check whether one was already created.
- the node object gets an extra 'void\* ctag' field, a custom tag to make a ross reference to the script side intermediate object. It can be set using ixmlNode_setCTag(), and read using ixmlNode_getCTag(). Whenever a new intermediate object is created, the ctag of the corresponding node should be set to point to this intermediate object.
- The tree structure traversal is done on the C side (looking up parents, children and siblings)
- Every intermediate object created should be kept in a list (preferably a key-value list, where the key is the pointer to the node and the value is the pointer to the intermediate object)
- when the callback is called, the node should be looked up in the list, the flag set to false, the pointer to the C-side node be cleared and on the C-side the ctag should be cleared.
- whenever the intermediate object is accessed and its flag is set to False, an error should be thrown that the XML document has been closed.

Freeing resources can be done in 2 ways, C side by simply calling the free node methods, or script side by the garbage collector of the script engine.

Script side steps:

- if the valid flag is set to False (XML document is closed), then the intermediate object can be destroyed, no further action.
- if the node has a parent, then the intermediate object can be destroyed after the ctag on the corresponding node has been cleared. Nothing needs to be freed on the C-side.
- if the node has no parent, then the node must be freed on the C side by calling the corresponding free node methods. This will result in a chain of callbacks closing the node and all underlying nodes.

## 16. Thanks

- To all the people listed in [the THANKS file](THANKS).
- To [JetBrains](https://www.jetbrains.com/?from=pupnp) for kindly providing us with open source licenses of their amazing products.

![JetBrains Logo](site/jetbrains.svg)
