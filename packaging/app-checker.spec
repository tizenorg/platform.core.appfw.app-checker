Name:	    app-checker
Summary:    App Checker
Version:    0.0.5
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

Requires(post): /sbin/ldconfig
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

CFLAGS+=" -fvisibility=hidden -fpic"
LDFLAGS+=" -Wl,--rpath=/usr/lib -Wl,--as-needed"

cmake . -DCMAKE_INSTALL_PREFIX=/usr

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%post -p /sbin/ldconfig

%post server -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%postun server -p /sbin/ldconfig


%files
#/usr/bin/ac_test
/usr/lib/libapp-checker.so.0
%attr(644, root, root) /usr/lib/libapp-checker.so.0.1.0

%files devel
/usr/lib/libapp-checker.so
/usr/lib/pkgconfig/app-checker.pc
/usr/include/app-checker/app-checker.h

%files server
/usr/lib/libapp-checker-server.so.0
%attr(644, root, root) /usr/lib/libapp-checker-server.so.0.1.0

%files server-devel
/usr/lib/libapp-checker-server.so
/usr/lib/pkgconfig/app-checker-server.pc
/usr/include/app-checker/app-checker-server.h

