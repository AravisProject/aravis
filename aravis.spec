Name:		aravis		
Version:	0.1.15
Release:	1%{?dist}
Summary:	Aravis digital video camera acquisition library

Group:		System/Libraries
License:	GPLv2+
URL:		http://live.gnome.org/Aravis
Source0:	aravis-%{version}.tar.bz2

BuildRequires:	pkgconfig(glib-2.0) >= 2.26
BuildRequires:	pkgconfig(gobject-2.0)
BuildRequires:	pkgconfig(gio-2.0)
BuildRequires:	pkgconfig(libxml-2.0)
BuildRequires:	pkgconfig(gthread-2.0)
BuildRequires:	pkgconfig(gtk+-3.0)
BuildRequires:	pkgconfig(libnotify)
BuildRequires:	pkgconfig(gstreamer-base-0.10) >= 0.10.31
BuildRequires:	pkgconfig(gstreamer-app-0.10)
BuildRequires:	pkgconfig(gstreamer-interfaces-0.10)
BuildRequires:	desktop-file-utils

Source10:	aravis.png

%global fullname %{name}-0.2

Requires:	pkgconfig(glib-2.0) >= 2.26
Requires:	pkgconfig(gobject-2.0)
Requires:	pkgconfig(gio-2.0)
Requires:	pkgconfig(libxml-2.0)
Requires:	pkgconfig(gthread-2.0)

%description
Aravis is a glib/gobject based library for video acquisition using Genicam cameras. It currently only implements an ethernet camera protocol used for industrial cameras. 

%package devel
Summary:	Aravis digital video camera acquisition library -- Development files
Group:		Development/Libraries
Requires:	%{name} = %{version}

%description devel
Aravis is a glib/gobject based library for video acquisition using Genicam cameras. It currently only implements an ethernet camera protocol used for industrial cameras. 

This package contains the development files for Aravis.

%package viewer
Summary:	Aravis digital video camera acquisition library -- Viewer
Group:		Development/Libraries
Requires:	%{name} = %{version}
Requires:	pkgconfig(libnotify)
Requires:	pkgconfig(gtk+-3.0)
Requires:	pkgconfig(gstreamer-base-0.10) >= 0.10.31
Requires:	pkgconfig(gstreamer-app-0.10)
Requires:	pkgconfig(gstreamer-interfaces-0.10)

%description viewer
Aravis is a glib/gobject based library for video acquisition using Genicam cameras. It currently only implements an ethernet camera protocol used for industrial cameras. 

This package contains the simple video viewer application.

%package gstreamer-plugin

Summary:	Aravis digital video camera acquisition library -- GStreamer plugin
Group:		Development/Libraries
Requires:	%{name} = %{version}
Requires:	pkgconfig(gstreamer-base-0.10) >= 0.10.31
Requires:	pkgconfig(gstreamer-app-0.10)
Requires:	pkgconfig(gstreamer-interfaces-0.10)

%description gstreamer-plugin
Aravis is a glib/gobject based library for video acquisition using Genicam cameras. It currently only implements an ethernet camera protocol used for industrial cameras. 

This package contains the GStreamer plugin.

%prep
%setup -q

%build
%configure --enable-gtk3 --enable-viewer --enable-notify --enable-gst-plugin
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}
%find_lang %{fullname}
desktop-file-install --vendor=""                                 \
       --dir=%{buildroot}%{_datadir}/applications/   \
       %{buildroot}%{_datadir}/applications/arv-viewer.desktop

%files -f %{fullname}.lang
%{_bindir}/arv-tool-0.2
%{_bindir}/arv-fake-gv-camera-0.2
%{_datadir}/%{fullname}/*.xml
%{_libdir}/lib%{fullname}*
%{_libdir}/girepository-1.0/*
/usr/doc/%{fullname}

%files devel
%{_datadir}/gtk-doc/html/%{fullname}
%{_includedir}/%{fullname}
%{_libdir}/pkgconfig/*
%{_datadir}/gir-1.0/*

%files viewer
%{_bindir}/arv-viewer
%{_datadir}/%{fullname}/*.ui
%{_datadir}/icons/hicolor/22x22/apps/*
%{_datadir}/icons/hicolor/32x32/apps/*
%{_datadir}/icons/hicolor/48x48/apps/*
%{_datadir}/icons/hicolor/256x256/apps/*
%{_datadir}/applications/arv-viewer.desktop

%post viewer
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache -q %{_datadir}/icons/hicolor;
fi
update-mime-database %{_datadir}/mime &> /dev/null || :
update-desktop-database &> /dev/null || :

%postun viewer
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache -q %{_datadir}/icons/hicolor;
fi
update-mime-database %{_datadir}/mime &> /dev/null || :
update-desktop-database &> /dev/null || :

%files gstreamer-plugin
%{_libdir}/gstreamer-0.10/*

%changelog

