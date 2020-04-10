QT       -= gui
TEMPLATE = lib

DEFINES += MATCHER_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += PRO_PWD=\\\"$$_PRO_FILE_PWD_\\\"
QMAKE_CXXFLAGS += -Wl,--stack,100000000
QMAKE_CXXFLAGS += -Wl,--heap,100000000
CONFIG += c++17
QMAKE_CFLAGS_ISYSTEM=

SOURCES += \
        matcher.cpp \
        bozorth3m.cpp \
        matcher_isoconverter.cpp

HEADERS += \
        matcher.h \
        matcher_global.h \ 
        matcher_config.h \
        bozorth3m.h \
        matcher_isoconverter.h
