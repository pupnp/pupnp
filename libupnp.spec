Version: 1.8.0
Summary: Universal Plug and Play (UPnP) SDK
Name: libupnp
Release: 1%{?dist}
License: BSD
Group: System Environment/Libraries
URL: http://www.libupnp.org/
Source: http://puzzle.dl.sourceforge.net/sourceforge/pupnp/%{name}-%{version}.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%define docdeveldir %{_docdir}/%{name}-devel-%{version}

%description
The Universal Plug and Play (UPnP) SDK for Linux provides 
support for building UPnP-compliant control points, devices, 
and bridges on Linux.

%package devel
Group: Development/Libraries
Summary: Include files needed for development with libupnp
Requires: libupnp = %{version}-%{release}

%description devel
The libupnp-devel package contains the files necessary for development with
the UPnP SDK libraries.

%prep
%setup -q

%build
%configure --with-documentation
make %{?_smp_mflags}

%install
test "$RPM_BUILD_ROOT" != "/" && rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

#create the doc devel dir
%{__mkdir_p} %{buildroot}%{docdeveldir}

#mv examples dir and pdf file to the doc devel dir
%{__mv} %{buildroot}%{docdir}/examples \
	%{buildroot}%{docdeveldir}/
%{__mv} %{buildroot}%{docdir}/UPnP_Programming_Guide.pdf \
	%{buildroot}%{docdeveldir}/
%{__mv} %{buildroot}%{docdir}/IXML_Programming_Guide.pdf \
	%{buildroot}%{docdeveldir}/
%{__mv} %{buildroot}%{docdir}/html \
	%{buildroot}%{docdeveldir}/

%{__rm} %{buildroot}%{_libdir}/{libixml.la,libthreadutil.la,libupnp.la}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc %{docdir}
%{_libdir}/libixml.so.*
%{_libdir}/libthreadutil.so.*
%{_libdir}/libupnp.so.*

%files devel
%defattr(0644,root,root,0755)
%doc %{docdeveldir}
%{_includedir}/upnp/
%{_libdir}/libixml.so
%{_libdir}/libthreadutil.so
%{_libdir}/libupnp.so
%{_libdir}/libixml.a
%{_libdir}/libthreadutil.a
%{_libdir}/libupnp.a
%{_libdir}/pkgconfig/libupnp.pc

%clean
rm -rf %{buildroot}

%changelog
* Mon Nov 19 2007 Marcelo Jimenez <mroberto@users.sourceforge.net> - 1.6.2-1
- Update to version 1.6.2

* Mon Nov 19 2007 Marcelo Jimenez <mroberto@users.sourceforge.net> - 1.4.7-1
- Update to version 1.4.7

* Fri Feb 02 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.2-1
- Update to version 1.4.2

* Wed Jul 05 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.1-1
- Update to version 1.4.1

* Fri Jun 23 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.0-3
- modified patch for x86_64 arch

* Fri Jun 23 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.0-2
- Add a patch for x86_64 arch

* Sun Jun 11 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.0-1
- Update to 1.4.0

* Sun Mar 05 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.3.1-1
- Update to 1.3.1

* Tue Feb 14 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.2.1a-6
- Rebuild for FC5

* Fri Feb 10 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.2.1a-5
- Rebuild for FC5

* Mon Jan  9 2006 Eric Tanguy 1.2.1a-4
- Include libupnp.so symlink in package to take care of non versioning of libupnp.so.1.2.1

* Sun Jan  8 2006 Paul Howarth 1.2.1a-3
- Disable stripping of object code for sane debuginfo generation
- Edit makefiles to hnnor RPM optflags
- Install libraries in %%{_libdir} rather than hardcoded /usr/lib
- Fix libupnp.so symlink
- Own directory %%{_includedir}/upnp
- Fix permissions in -devel package

* Fri Jan 06 2006 Eric Tanguy 1.2.1a-2
- Use 'install -p' to preserve timestamps
- Devel now require full version-release of main package

* Thu Dec 22 2005 Eric Tanguy 1.2.1a-1
- Modify spec file from 
http://rpm.pbone.net/index.php3/stat/4/idpl/2378737/com/libupnp-1.2.1a_DSM320-3.i386.rpm.html

