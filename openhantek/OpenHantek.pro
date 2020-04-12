# Helper functions
defineReplace(surround) {
	elements = $$1
	prefix = $$2
	postfix = $$3
	result =
	for(element, elements) {
		result += "$${prefix}$${element}$${postfix}"
	}
	return($${result})
}

TEMPLATE = app

# Configuration
CONFIG += \
	warn_on \
	qt
QT += opengl printsupport
LIBS += -lfftw3

# Source files
SOURCES += \
	src/colorbox.cpp \
	src/configdialog.cpp \
	src/configpages.cpp \
	src/dataanalyzer.cpp \
	src/dockwindows.cpp \
	src/dso.cpp \
	src/dsocontrol.cpp \
	src/dsowidget.cpp \
	src/exporter.cpp \
	src/glgenerator.cpp \
	src/glscope.cpp \
	src/helper.cpp \
	src/levelslider.cpp \
	src/main.cpp \
	src/openhantek.cpp \
	src/settings.cpp \
	src/sispinbox.cpp \
	src/hardcontrol.cpp \
	src/hantek/control.cpp \
	src/hantek/device.cpp \
	src/hantek/types.cpp
HEADERS += \
	src/colorbox.h \
	src/configdialog.h \
	src/configpages.h \
	src/dataanalyzer.h \
	src/dockwindows.h \
	src/dso.h \
	src/dsocontrol.h \
	src/dsowidget.h \
	src/exporter.h \
	src/glscope.h \
	src/glgenerator.h \
	src/helper.h \
	src/levelslider.h \
	src/openhantek.h \
	src/settings.h \
	src/sispinbox.h \
	src/hardcontrol.h \
	src/hantek/control.h \
	src/hantek/device.h \
	src/hantek/types.h

# Ressource files
RESOURCES += \
	res/application.qrc \
	res/configdialog.qrc

# Doxygen files
DOXYFILES += \
	Doxyfile \
	mainpage.dox \
	roadmap.dox

# Files copied into the distribution package
DISTFILES += \
	ChangeLog \
	COPYING \
	INSTALL \
	res/images/*.png \
	res/images/*.icns \
	res/images/*.svg \
	$${DOXYFILES}

# Program version
VERSION = 0.2.0

# Destination directory for built binaries
DESTDIR = bin

# Prefix for installation
PREFIX = $$(PREFIX)

# Build directories
OBJECTS_DIR = build/obj
UI_DIR = build/ui
MOC_DIR = build/moc

# libusb version
LIBUSB_VERSION = $$(LIBUSB_VERSION)
contains(LIBUSB_VERSION, 0) {
	LIBS += -lusb
}
else { 
	LIBUSB_VERSION = 1
	LIBS += -lusb-1.0
}
DEFINES += LIBUSB_VERSION=$${LIBUSB_VERSION}

# Debug output
CONFIG(debug, debug|release): DEFINES += DEBUG
else: DEFINES += QT_NO_DEBUG_OUTPUT

# Quoted include directories
INCLUDEPATH_QUOTE = "$${IN_PWD}/src"

# Include directory
QMAKE_CXXFLAGS += "-iquote $${INCLUDEPATH_QUOTE}"

# Settings for different operating systems
unix:!macx { 
	isEmpty(PREFIX): PREFIX = /usr/local
	TARGET = openhantek
	
	# Installation directories
	target.path = $${PREFIX}/bin
	contains(LIBUSB_VERSION, 0) {
		INCLUDEPATH += /usr/include/libusb
	}
	else {
		INCLUDEPATH += /usr/include/libusb-1.0
	}
	DEFINES += \
		OS_UNIX VERSION=\\\"$${VERSION}\\\"
}
macx { 
	isEmpty(PREFIX): PREFIX = OpenHantek.app
	TARGET = OpenHantek
	
	# Installation directories
	target.path = $${PREFIX}/Contents/MacOS
	INCLUDEPATH += $${INCLUDEPATH_QUOTE}
	LIBS += -framework IOKit -framework CoreFoundation
	ICON = res/images/openhantek.icns
	DEFINES += \
		OS_DARWIN VERSION=\\\"$${VERSION}\\\"
}
win32 { 
	isEmpty(PREFIX): PREFIX = OpenHantek
	TARGET = OpenHantek
	
	# Installation directories
	target.path = $${PREFIX}
	INCLUDEPATH += $${INCLUDEPATH_QUOTE} \
		OS_WINDOWS VERSION=\\\"$${VERSION}\\\"
}
INSTALLS += \
	target \

# Custom target "cppcheck" for Cppcheck
INCLUDEPARAMETERS = $$surround($${INCLUDEPATH} $${INCLUDEPATH_QUOTE}, "-I \"", "\"")
cppcheck.commands = "cppcheck --enable=all $${INCLUDEPARAMETERS} -q $${SOURCES} --template=\"{file}:{line}: {severity}: {message}\""
cppcheck.depends = $${SOURCES}

# Custom target "doc" for Doxygen
doxygen.target = "doc"
doxygen.commands = "rm -r doc/; env DEFINES=\"$${DEFINES}\" doxygen Doxyfile"
doxygen.depends = \
	$${SOURCES} \
	$${HEADERS} \
	$${DOXYFILES}

QMAKE_EXTRA_TARGETS += \
	cppcheck \
	doxygen
