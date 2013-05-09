Summary: Input library for GGI
Name: libgii
Version: 1.0.2
Release: 1
Group: System Environment/Libraries
Source0: ftp://ftp.ggi-project.org/pub/ggi/%name-%{version}.tar.gz
URL: http://www.ggi-project.org/
License: LGPL
BuildRoot: %{_tmppath}/%{name}-%{version}-root


%description
Input library for GGI


%package devel
Summary: Header files and libraries for developing apps which will use libgii.
Group: Development/Libraries
Requires: %{name} = %{version}


%description devel
Header files and libraries for developing apps which will use libgii.


%prep
%setup -q


%build
%configure
gmake


%install
[ -e "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
%makeinstall


%clean
[ -e "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%doc ChangeLog ChangeLog.1999 FAQ NEWS README TODO
%doc doc/env.txt doc/inputs.txt doc/docbook/*.sgml
%config %{_sysconfdir}/ggi
%{_bindir}/*
%{_libdir}/*.so.*
%{_libdir}/ggi
%{_mandir}/man1/*
%{_mandir}/man7/*


%files devel
%defattr(-,root,root)
%{_libdir}/*.la
%{_libdir}/*.so
%{_includedir}/*
%{_mandir}/man3/*
%{_mandir}/man9/*


%changelog
* Wed Aug 22 2001 Thayne Harbaugh <thayne@plug.org>
- added missing man9 and TODO files

* Mon Jul 02 2001 Thayne Harbaugh <thayne@plug.org>
- initial libgii.spec

