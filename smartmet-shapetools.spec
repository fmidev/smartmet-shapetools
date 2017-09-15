%define BINNAME shapetools
%define RPMNAME smartmet-%{BINNAME}
Summary: Command line tools for handling ESRI shapefiles
Name: %{RPMNAME}
Version: 17.9.13
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Tools
URL: https://github.com/fmidev/smartmet-shapetools
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost-devel
BuildRequires: libjpeg
BuildRequires: libjpeg-devel
BuildRequires: libpng-devel
BuildRequires: smartmet-library-imagine-devel >= 17.8.28
BuildRequires: smartmet-library-newbase-devel >= 17.8.28
BuildRequires: smartmet-library-macgyver-devel >= 17.8.28
Requires: smartmet-library-imagine >= 17.8.28
Requires: smartmet-library-newbase >= 17.8.28
Requires: smartmet-library-macgyver >= 17.8.28
Requires: boost-iostreams
Requires: boost-filesystem
Requires: boost-program-options
Requires: boost-system
Requires: glibc
Requires: libgcc
Requires: libjpeg
Requires: libpng
Requires: libstdc++
Requires: zlib
Provides: amalgamate
Provides: compositealpha
Provides: etopo2shape
Provides: grads2shape
Provides: gradsdump
Provides: gshhs2grads
Provides: gshhs2shape
Provides: lights2shape
Provides: shape2grads
Provides: shape2ps
Provides: shape2svg
Provides: shape2triangle
Provides: shape2xml
Provides: shapedump
Provides: shapefilter
Provides: shapefind
Provides: shapepack
Provides: shapepoints
Provides: shapeproject
Provides: svg2shape
Provides: triangle2shape

%description
FMI shapetools

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{RPMNAME}
 
%build
make %{_smp_mflags} 

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,-)
/usr/bin/compositealpha
/usr/bin/shapefilter 
/usr/bin/shapeproject 
/usr/bin/shapepoints 
/usr/bin/shape2grads 
/usr/bin/grads2shape 
/usr/bin/gradsdump 
/usr/bin/gshhs2grads 
/usr/bin/gshhs2shape 
/usr/bin/shape2ps 
/usr/bin/shape2svg
/usr/bin/shape2xml 
/usr/bin/shapedump 
/usr/bin/triangle2shape 
/usr/bin/shape2triangle 
/usr/bin/amalgamate 
/usr/bin/etopo2shape 
/usr/bin/lights2shape
/usr/bin/shapepack
/usr/bin/shapepick
/usr/bin/shapefind
/usr/bin/svg2shape

%changelog
* Wed Sep 13 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.9.13-1.fmi
- Fixed license to be MIT

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Mon Feb 13 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.13-1.fmi
- Repackaged due to newbase API changes

* Wed Jan 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.11-1.fmi
- Switched to FMI open source naming conventions

* Thu Aug  4 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.8.4-1.fmi
- Recompiled with the latest newbase

* Thu Jan 21 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.21-1.fmi
- newbase API changed

* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed

* Mon Mar 30 2015  <mheiskan@centos7.fmi.fi> - 15.3.30-1.fmi
- Use dynamic linking of smartmet libraries

* Tue Jan 21 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.1.21-1.fmi
- shape2ps now adds a clip path the size of the bounding box for AI working
- shape2ps now generates the bounding box path with command boundingbox

* Tue Dec 10 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.12.10-1.fmi
- Fixed shape2svg to handle multipart shapes correctly

* Mon Dec  9 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.12.9-1.fmi
- Added svg2shape program

* Fri Sep 27 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.9.27-1.fmi
- Fixed shapefilter and shapepoints not to crash
- Added subshape command to shape2ps

* Fri Aug  2 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.8.2-2.fmi
- Fixed problems in handling long lines at the poles

* Fri Aug  2 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.8.2-1.fmi
- Added support for Pacific views
- Enabled Pacific views of data

* Wed Jul  3 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.7.3-1.fmi
- Update to boost 1.54

* Thu Jul  5 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.7.5-1.fmi
- Migration to boost 1.50

* Fri Dec 16 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.12.16-2.el6.fmi
- Recompiled due to a macgyver bug fix in timeone handling

* Fri Dec 16 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.12.16-1.el6.fmi
- Added shapepick program

* Fri Nov 11 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.11.11-2.el5.fmi
- Added graticule command to shape2ps

* Fri Nov 11 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.11.11-1.el5.fmi
- Added option -z to shapepack

* Thu Oct 20 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.10.20-1.el5.fmi
- Added compositealpha

* Tue Oct 18 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.10.18-1.el5.fmi
- Added shape2svg

* Mon Jul  5 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.5-1.el5.fmi
- Upgrade to newbase 10.7.5-2

* Fri Jan 15 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.1.15-1.el5.fmi
- Upgrade to boost 1.41

* Wed Mar  4 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.3.4-1.el5.fmi
- Newbase preprocessor fixes

* Fri Nov 18 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.11.18-1.el5.fmi
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
