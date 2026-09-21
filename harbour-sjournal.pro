# SJournal — handwritten notes for Sailfish OS.
# The document core has no Silica in it; only src/app and src/render know QML.

TARGET = harbour-sjournal

CONFIG += sailfishapp sailfishapp_i18n c++11

QT += quick concurrent

# .xopp is gzipped XML; zlib's gz* API reads plain files too.
LIBS += -lz

# Keep the build machine's source paths out of the binary.
QMAKE_CXXFLAGS += -ffile-prefix-map=$$PWD=/build

# Binary hardening: notes carry embedded images from untrusted sources.
QMAKE_CXXFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector-strong
QMAKE_LFLAGS += -Wl,-z,relro -Wl,-z,now

INCLUDEPATH += src lib/potrace lib/zinnia

# potrace, vendored under lib/potrace (GPL-2.0-or-later); config.h is a stub.
DEFINES += HAVE_CONFIG_H
SOURCES += \
    lib/potrace/potracelib.c \
    lib/potrace/curve.c \
    lib/potrace/decompose.c \
    lib/potrace/trace.c

# zinnia, vendored under lib/zinnia (new BSD); a quoted config.h resolves per directory.
SOURCES += \
    lib/zinnia/character.cpp \
    lib/zinnia/feature.cpp \
    lib/zinnia/libzinnia.cpp \
    lib/zinnia/param.cpp \
    lib/zinnia/recognizer.cpp \
    lib/zinnia/sexp.cpp \
    lib/zinnia/svm.cpp \
    lib/zinnia/trainer.cpp

SOURCES += \
    src/harbour-sjournal.cpp \
    src/model/gradient.cpp \
    src/model/stroke.cpp \
    src/model/path.cpp \
    src/model/textitem.cpp \
    src/model/imageitem.cpp \
    src/model/layer.cpp \
    src/model/page.cpp \
    src/model/document.cpp \
    src/io/gzfile.cpp \
    src/io/xoppreader.cpp \
    src/io/xoppwriter.cpp \
    src/io/svgexporter.cpp \
    src/render/strokegeometry.cpp \
    src/render/elementpainter.cpp \
    src/render/overlaypainter.cpp \
    src/render/canvasitem.cpp \
    src/tools/shaperecognizer.cpp \
    src/tools/shapefactory.cpp \
    src/tools/bitmaptracer.cpp \
    src/tools/strokebuilder.cpp \
    src/tools/curvefitter.cpp \
    src/tools/handwriting.cpp \
    src/tools/pathops.cpp \
    src/app/undostack.cpp \
    src/app/layermodel.cpp \
    src/app/imagetools.cpp \
    src/app/exporter.cpp \
    src/app/documentstore.cpp \
    src/app/selection.cpp \
    src/app/nodeeditor.cpp \
    src/app/pathtools.cpp \
    src/app/styletools.cpp \
    src/app/selectiontransform.cpp \
    src/app/toolsettings.cpp \
    src/app/documentcontroller.cpp \
    src/app/notebookmodel.cpp \
    src/app/languagesetting.cpp

HEADERS += \
    src/model/element.h \
    src/model/gradient.h \
    src/model/stroke.h \
    src/model/path.h \
    src/model/textitem.h \
    src/model/imageitem.h \
    src/model/layer.h \
    src/model/page.h \
    src/model/document.h \
    src/io/gzfile.h \
    src/io/xoppreader.h \
    src/io/xoppwriter.h \
    src/io/svgexporter.h \
    src/render/strokegeometry.h \
    src/render/elementpainter.h \
    src/render/overlaypainter.h \
    src/render/canvasitem.h \
    src/tools/shaperecognizer.h \
    src/tools/shapefactory.h \
    src/tools/bitmaptracer.h \
    src/tools/strokebuilder.h \
    src/tools/curvefitter.h \
    src/tools/handwriting.h \
    src/tools/pathops.h \
    src/app/undostack.h \
    src/app/layermodel.h \
    src/app/imagetools.h \
    src/app/exporter.h \
    src/app/documentstore.h \
    src/app/selection.h \
    src/app/nodeeditor.h \
    src/app/pathtools.h \
    src/app/styletools.h \
    src/app/selectiontransform.h \
    src/app/toolsettings.h \
    src/app/documentcontroller.h \
    src/app/notebookmodel.h \
    src/app/languagesetting.h

DISTFILES += \
    harbour-sjournal.desktop \
    rpm/harbour-sjournal.spec \
    qml/harbour-sjournal.qml \
    qml/cover/CoverPage.qml \
    qml/pages/*.qml \
    qml/components/*.qml \
    translations/*.ts

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

TRANSLATIONS += translations/harbour-sjournal-de.ts \
    translations/harbour-sjournal-en.ts
