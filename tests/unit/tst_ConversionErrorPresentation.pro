QT = core testlib

TARGET = tst_ConversionErrorPresentation

include(unit_test.pri)

SOURCES += \
    tst_ConversionErrorPresentation.cpp \
    ../../src/ui/ConversionErrorPresentation.cpp

HEADERS += \
    ../../src/model/ConversionError.h \
    ../../src/ui/ConversionErrorPresentation.h
