Name:	    app-checker
Summary:    App Checker
Version:    0.0.16
Release:    1
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires: cmake
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(glib-2.0)


%description
libapp-checker

%package devel
Summary:    App Checker
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
libapp-checker (developement files)

%package server
Summary:    App Checker Server
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description server
libapp-checker server

%package server-devel
Summary:    App Checker Server
Group:      System/Libraries
Requires:   %{name}-server = %{version}-%{release}

%description server-devel
libapp-checker server (developement files)



%prep
%setup -q


%build

%cmake . 

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}%{_libdir}/ac-plugins


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest app-checker.manifest
%defattr(-,root,root,-)
%{_libdir}/libapp-checker.so.0
%{_libdir}/libapp-checker.so.0.1.0
%{_libdir}/ac-plugins

%files devel
%defattr(-,root,root,-)
%{_libdir}/libapp-checker.so
%{_libdir}/pkgconfig/app-checker.pc
/usr/include/app-checker/app-checker.h

%files server
%manifest app-checker.manifest
%defattr(-,root,root,-)
%{_libdir}/libapp-checker-server.so.0
%{_libdir}/libapp-checker-server.so.0.1.0

%files server-devel
%defattr(-,root,root,-)
%{_libdir}/libapp-checker-server.so
%{_libdir}/pkgconfig/app-checker-server.pc
/usr/include/app-checker/app-checker-server.h

