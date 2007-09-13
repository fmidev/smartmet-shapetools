Summary: shapetools
Name: shapetools
Version: 1.0
Release: 1
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}
Requires: boost, newbase >= 1.0-1, imagine >= 1.0-1,  libjpeg, libjpeg-devel, libpng-devel >= 1.2.2, libpng10 => 1.0, zlib >= 1.1.4, zlib-devel >= 1.1.4

%description
FMI shapetools

%prep
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT

%setup -q -n %{name}
 
%build
make %{_smp_mflags} 

%install
make install prefix="${RPM_BUILD_ROOT}"

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,www,www,0775)
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
* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 
- Initial build.

