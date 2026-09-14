QT = core testlib

TARGET = tst_DocumentFileOperations

include(unit_test.pri)

SOURCES += \
    tst_DocumentFileOperations.cpp \
    ../../src/io/DocumentFileOperations.cpp

HEADERS += \
    ../../src/io/DocumentFileOperations.h
