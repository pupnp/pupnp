# Microsoft Developer Studio Project File - Name="libupnp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libupnp - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "libupnp.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausf?hren von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "libupnp.mak" CFG="libupnp - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "libupnp - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libupnp - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libupnp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBUPNP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\pthreads\include" /I "..\ixml\src\inc" /I "..\ixml\inc" /I "..\threadutil\inc" /I "..\upnp\inc" /I "..\upnp\src\inc" /I ".\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBUPNP_EXPORTS" /D "PTW32_STATIC_LIB" /D "UPNP_STATIC_LIB" /D "UPNP_USE_MSVCPP" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\pthreads\lib\pthreadvc2.lib ws2_32.lib /nologo /dll /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "libupnp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBUPNP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\pthreads\include" /I "..\ixml\src\inc" /I "..\ixml\inc" /I "..\threadutil\inc" /I "..\upnp\inc" /I "..\upnp\src\inc" /I "..\build\inc" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBUPNP_EXPORTS" /D "UPNP_USE_MSVCPP" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\pthreads\lib\pthreadvc2.lib ws2_32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libupnp - Win32 Release"
# Name "libupnp - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\ixml\src\attr.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\client_table\client_table.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\document.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\element.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\FreeList.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\gena\gena_callback2.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\gena\gena_ctrlpt.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\gena\gena_device.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\httpparser.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\httpreadwrite.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\iasnprintf.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inet_pton.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\ixml.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\ixmlmembuf.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\ixmlparser.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\LinkedList.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\uuid\md5.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\membuffer.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\miniserver\miniserver.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\namedNodeMap.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\node.c
# End Source File
# Begin Source File

SOURCE=..\ixml\src\nodeList.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\parsetools.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\service_table\service_table.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\soap\soap_common.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\soap\soap_ctrlpt.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\soap\soap_device.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\sock.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\ssdp\ssdp_ctrlpt.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\ssdp\ssdp_device.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\ssdp\ssdp_server.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\statcodes.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\strintmap.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\uuid\sysdep.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\ThreadPool.c
# End Source File
# Begin Source File

SOURCE=..\threadutil\src\TimerThread.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\upnp_timeout.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\api\upnpapi.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\api\upnptools.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\uri\uri.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\urlconfig\urlconfig.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\util\util.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\uuid\uuid.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\genlib\net\http\webserver.c
# End Source File
# Begin Source File

SOURCE=..\upnp\src\win_dll.c
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\upnp\src\inc\client_table.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\config.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\gena.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\gena_ctrlpt.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\gena_device.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\global.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\gmtdate.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\http_client.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\httpparser.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\httpreadwrite.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\inet_pton.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\md5.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\membuffer.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\miniserver.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\netall.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\parsetools.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\server.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\service_table.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\soaplib.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\sock.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\ssdplib.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\statcodes.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\statuscodes.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\strintmap.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\sysdep.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\unixutil.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\upnp_timeout.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\upnpapi.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\uri.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\urlconfig.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\util.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\utilall.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\uuid.h
# End Source File
# Begin Source File

SOURCE=..\upnp\src\inc\webserver.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\..\..\libupnp_win32.patch
# End Source File
# End Target
# End Project
