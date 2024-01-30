%define BINNAME shapetools
%define RPMNAME smartmet-%{BINNAME}
Summary: Command line tools for handling ESRI shapefiles
Name: %{RPMNAME}
Version: 23.7.28
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Tools
URL: https://github.com/fmidev/smartmet-shapetools
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)

%if 0%{?rhel} && 0%{rhel} < 9
%define smartmet_boost boost169
%else
%define smartmet_boost boost
%endif

BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: %{smartmet_boost}-devel
BuildRequires: libjpeg
BuildRequires: libjpeg-devel
BuildRequires: libpng-devel
BuildRequires: gdal35-devel
BuildRequires: smartmet-library-imagine-devel >= 23.7.10
BuildRequires: smartmet-library-newbase-devel >= 23.7.28
BuildRequires: smartmet-library-macgyver-devel >= 23.7.28
BuildRequires: smartmet-library-gis-devel >= 23.7.10
Requires: smartmet-library-imagine >= 23.7.10
Requires: smartmet-library-newbase >= 23.7.28
Requires: smartmet-library-macgyver >= 23.7.28
Requires: smartmet-library-gis >= 23.7.10
Requires: %{smartmet_boost}-iostreams
Requires: %{smartmet_boost}-filesystem
Requires: %{smartmet_boost}-program-options
Requires: %{smartmet_boost}-system
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
* Fri Jul 28 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.28-1.fmi
- Repackage due to bulk ABI changes in macgyver/newbase/spine

* Wed Jul 12 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.12-1.fmi
- Use postgresql 15, gdal 3.5, geos 3.11 and proj-9.0

* Mon Jun 20 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.6.20-1.fmi
- Add support for RHEL9, upgrade libpqxx to 7.7.0 (rhel8+) and fmt to 8.1.1

* Tue May 24 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.24-1.fmi
- Repackaged due to NFmiArea ABI changes

* Fri May 20 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.20-1.fmi
- Repackaged due to ABI changes to newbase LatLon methods

* Wed May 18 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.18-1.fmi
- Removed obsolete #ifdef WGS84 statements

* Fri Jan 21 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.1.21-1.fmi
- Repackage due to upgrade of packages from PGDG repo: gdal-3.4, geos-3.10, proj-8.2

* Tue Dec  7 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.12.7-1.fmi
- Update to postgresql 13 and gdal 3.3

* Thu May  6 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.5.6-1.fmi
- Repackaged due to ABI changes in NFmiAzimuthalArea

* Fri Feb 19 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.19-1.fmi
- Repackaged due to newbase ABI changes

* Mon Feb 15 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.15-1.fmi
- Ported to use new interpolation APIs

* Mon Jan 25 2021 Andris Pavenis <andris.pavenis@fmi.fi> - 21.1.25-1.fmi
- Build update: use makefile.inc

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgraded to Boost 1.69

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Repackaged due to newbase API changes

* Thu Oct 31 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Rebuilt due to newbase API/ABI changes

* Fri Sep 27 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.27-1.fmi
- Repackaged due to ABI changes in SmartMet libraries

* Mon Sep 10 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.10-1.fmi
- Optimized shapepack for speed

* Thu Jul 26 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.26-1.fmi
- Prefer nullptr over NULL

* Wed May  2 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.5.2-1.fmi
- Repackaged since newbase NFmiEnumConverter ABI changed

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Fri Sep 22 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.9.22-1.fmi
- Proper handling of M- and Z-type ESRI shapes

* Thu Sep 21 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.9.21-1.fmi
- Added support for handling shapes with dBASE Date fields in them

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

* Tue Nov 18 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.11.18-1.el5.fmi
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
