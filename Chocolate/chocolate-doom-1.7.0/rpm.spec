
Name: chocolate-doom
Summary: Conservative Doom source port
Version: 1.7.0
Release: 1
Source: http://mesh.dl.sourceforge.net/project/chocolate-doom/chocolate-doom/1.7.0/chocolate-doom-1.7.0.tar.gz
URL: http://www.chocolate-doom.org/
Group: Amusements/Games
BuildRoot: /var/tmp/chocolate-doom-buildroot
License: GNU General Public License, version 2
Packager: Simon Howard <fraggle@gmail.com>
Prefix: %{_prefix}
Autoreq: 0
Requires: libSDL-1.2.so.0, libSDL_mixer-1.2.so.0, libSDL_net-1.2.so.0

%description
%(sed -n "/==/ q; p " < README)

See http://www.chocolate-doom.org/ for more information.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q

%build
./configure \
 	--prefix=/usr \
	--exec-prefix=/usr \
	--bindir=/usr/bin \
	--sbindir=/usr/sbin \
	--sysconfdir=/etc \
	--datadir=/usr/share \
	--includedir=/usr/include \
	--libdir=/usr/lib \
	--libexecdir=/usr/lib \
	--localstatedir=/var/lib \
	--sharedstatedir=/usr/com \
	--mandir=/usr/share/man \
	--infodir=/usr/share/info
make

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc %{_mandir}/man5/*
%doc %{_mandir}/man6/*
/usr/share/doc/chocolate-doom/*
/usr/games/*
/usr/share/icons/*
/usr/share/applications/*

