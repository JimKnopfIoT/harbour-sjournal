# Neutral packaging metadata — no personal identifiers.
# The build host's name would otherwise end up in the RPM header.
%define _buildhost reproducible-builder

Name:       harbour-sjournal
Summary:    Handwritten notes and sketches for Sailfish OS
Version:    0.1.1
Release:    1
# GPLv3 for the application; the vendored potrace tracing core is
# GPL-2.0-or-later, which combines into GPLv3. See THIRD-PARTY.md.
License:    GPLv3 and GPLv2+
URL:        https://github.com/JimKnopfIoT/harbour-sjournal
Source0:    %{name}-%{version}.tar.bz2
Vendor:     SJournal contributors
Packager:   SJournal contributors

Requires:   sailfishsilica-qt5
# The layer thumbnails and the colour picker use QtGraphicalEffects.
Requires:   qt5-qtgraphicaleffects
# Picking a photo for an image layer, and picking a folder on export.
Requires:   sailfish-components-pickers-qt5
Requires:   sailfish-components-gallery-qt5
Requires:   qt5-qtdocgallery
Requires:   sailfish-content-graphics
# Export goes out through the share sheet.
Requires:   sailfishshare-components
# The image browser lists a folder with Qt.labs.folderlistmodel. The system
# pickers are not usable here: ImagePickerPage needs Tracker 1 while the device
# runs Tracker 3, and FilePickerPage opens on a partition list that needs
# udisks2 through Sailjail.
Requires:   qt5-qtdeclarative-import-folderlistmodel

BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(zlib)
BuildRequires: desktop-file-utils

%description
SJournal is a note taking and sketching application. Notes are stored as
vector strokes in the .xopp format, so the same file opens in Xournal++ on the
desktop. Photos can be placed as their own layer and drawn on, freehand shapes
are recognised and straightened, and any page exports to SVG with its layers
intact.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install
# sailfishapp.prf turns qmake's own strip off, so do it here.
strip %{buildroot}%{_bindir}/%{name}
desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
  %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog
* Mon Sep 21 2026 SJournal contributors 0.1.1-1
- Fix: tapping an image in the image picker did nothing.
- Fix: images with a transparent background were invisible in the picker.
- Counts read "1 item", not "1 items"; new layers are named in the app's language.
- Language can be set to English or German under About.

* Wed Sep 09 2026 SJournal contributors 0.1.0-1
- First build: .xopp notes, layers, shape recognition, SVG export.
