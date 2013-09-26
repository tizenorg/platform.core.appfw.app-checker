Name:       app-checker
Summary:    App Checker
Version:    0.0.16
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: app-checker.manifest
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
libapp-checker (development files)

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
libapp-checker server (development files)


%prep
%setup -q
cp %{SOURCE1001} .


%build

%cmake .

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}%{_libdir}/ac-plugins

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
cp LICENSE %{buildroot}/usr/share/license/%{name}-devel
cp LICENSE %{buildroot}/usr/share/license/%{name}-server
cp LICENSE %{buildroot}/usr/share/license/%{name}-server-devel

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libapp-checker.so.0
%{_libdir}/libapp-checker.so.0.1.0
%{_libdir}/ac-plugins
/usr/share/license/%{name}

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libapp-checker.so
%{_libdir}/pkgconfig/app-checker.pc
%{_includedir}/app-checker/app-checker.h
/usr/share/license/%{name}-devel

%files server
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libapp-checker-server.so.0
%{_libdir}/libapp-checker-server.so.0.1.0
/usr/share/license/%{name}-server

%files server-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libapp-checker-server.so
%{_libdir}/pkgconfig/app-checker-server.pc
%{_includedir}/app-checker/app-checker-server.h
/usr/share/license/%{name}-server-devel

