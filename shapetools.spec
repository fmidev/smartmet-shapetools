%define BINNAME shapetools
Summary: shapetools
Name: smartmet-%{BINNAME}
Version: 1.0.1
Release: 2.el5.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase >= 1.0.1-1, libsmartmet-imagine >= 1.0.1-1,  libjpeg, libjpeg-devel, libpng-devel
Requires: glibc, libgcc, libjpeg, libpng,  libstdc++, zlib
Provides: shapefilter shapeproject shapepoints shape2grads grads2shape gradsdump gshhs2grads gshhs2shape shape2ps shape2xml shapedump triangle2shape shape2triangle amalgamate etopo2shape lights2shape

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


%changelog
* Thu Sep 13 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-2.el5.fmi
- Improved make system
* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 
- Initial build.
