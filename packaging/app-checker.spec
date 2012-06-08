Name:	    app-checker
Summary:    App Checker
Version:    0.0.8
Release:    2
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/app-checker.manifest 

Requires(post): /sbin/ldconfig
Requires(post): /bin/mkdir
Requires(postun): /sbin/ldconfig


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
cp %{SOURCE1001} .

CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" cmake . -DCMAKE_INSTALL_PREFIX=/usr

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%post

/sbin/ldconfig
mkdir -p /usr/lib/ac-plugins

%postun -p /sbin/ldconfig


%files
%manifest app-checker.manifest
%defattr(-,root,root,-)
/usr/lib/libapp-checker.so.0
/usr/lib/libapp-checker.so.0.1.0

%files devel
%manifest app-checker.manifest
%defattr(-,root,root,-)
/usr/lib/libapp-checker.so
/usr/lib/pkgconfig/app-checker.pc
/usr/include/app-checker/app-checker.h

%files server
%manifest app-checker.manifest
%defattr(-,root,root,-)
/usr/lib/libapp-checker-server.so.0
/usr/lib/libapp-checker-server.so.0.1.0

%files server-devel
%manifest app-checker.manifest
%defattr(-,root,root,-)
/usr/lib/libapp-checker-server.so
/usr/lib/pkgconfig/app-checker-server.pc
/usr/include/app-checker/app-checker-server.h

