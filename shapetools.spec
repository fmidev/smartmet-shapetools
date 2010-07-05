%define BINNAME shapetools
Summary: shapetools
Name: smartmet-%{BINNAME}
Version: 10.7.5
Release: 1.el5.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: boost-devel
BuildRequires: libjpeg
BuildRequires: libjpeg-devel
BuildRequires: libpng-devel
BuildRequires: libsmartmet-imagine >= 10.7.5-2
BuildRequires: libsmartmet-newbase >= 10.7.5-2
Provides: amalgamate
Provides: etopo2shape
Provides: grads2shape
Provides: gradsdump
Provides: gshhs2grads
Provides: gshhs2shape
Provides: lights2shape
Provides: shape2grads
Provides: shape2ps
Provides: shape2triangle
Provides: shape2xml
Provides: shapedump
Provides: shapefilter
Provides: shapefind
Provides: shapepack
Provides: shapepoints
Provides: shapeproject
Provides: triangle2shape
Requires: glibc
Requires: libgcc
Requires: libjpeg
Requires: libpng
Requires: libstdc++
Requires: zlib

%description
FMI shapetools

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{BINNAME}
 
%build
make %{_smp_mflags} 

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0775)
/usr/bin/shapefilter 
/usr/bin/shapeproject 
/usr/bin/shapepoints 
/usr/bin/shape2grads 
/usr/bin/grads2shape 
/usr/bin/gradsdump 
/usr/bin/gshhs2grads 
/usr/bin/gshhs2shape 
/usr/bin/shape2ps 
/usr/bin/shape2xml 
/usr/bin/shapedump 
/usr/bin/triangle2shape 
/usr/bin/shape2triangle 
/usr/bin/amalgamate 
/usr/bin/etopo2shape 
/usr/bin/lights2shape
/usr/bin/shapepack
/usr/bin/shapefind

%changelog
* Mon Jul  5 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.5-1.el5.fmi
- Upgrade to newbase 10.7.5-2
* Fri Jan 15 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.1.15-1.el5.fmi
- Upgrade to boost 1.41
* Wed Mar  4 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.3.4-1.el5.fmi
- Newbase preprocessor fixes
* Tue Nov 14 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.11.18-1.el5.fmi
- Fixed polygon clipping in Polyline
* Thu Nov 13 2008 pkeranen <pekka.keranen@fmi.fi> - 8.11.13-1.el5.fmi
- Imagine shapefile support improved
* Mon Sep 29 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.29-1.el5.fmi
- Newbase headers changed
* Mon Sep 22 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.22-1.el5.fmi
- Linked statically with boost 1.36
* Mon Dec 31 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.5-1.el5.fmi
- Added polygon support to shapefind
* Wed Dec 19 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.4-1.el5.fmi
- Added -l option to shapefind
* Fri Dec 14 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.3-3.el5.fmi
- Added libbz2 linkage for RHEL3 releases
* Fri Dec 14 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.3-2.el5.fmi
- Upgraded imagine dependency
* Fri Dec 14 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.3-1.el5.fmi
- Added shapefind program
* Mon Sep 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-4.el5.fmi
- Fixed "make depend".
* Fri Sep 14 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-3.el5.fmi
- Added "make tag" feature.
* Thu Sep 13 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-2.el5.fmi
- Improved make system.
* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 
- Initial build.
