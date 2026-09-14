QT = core testlib

TARGET = tst_DocumentController

include(component_test.pri)

SOURCES += \
    tst_DocumentController.cpp \
    ../../src/controller/DocumentController.cpp \
    ../../src/markitdown/MarkItDownExecutableResolver.cpp \
    ../../src/markitdown/MarkItDownManager.cpp \
    ../../src/markitdown/ProcessRunner.cpp

HEADERS += \
    ../../src/controller/DocumentController.h \
    ../../src/markitdown/IMarkItDownManager.h \
    ../../src/markitdown/MarkItDownExecutableResolver.h \
    ../../src/markitdown/MarkItDownManager.h \
    ../../src/markitdown/ProcessRunner.h \
    ../../src/model/ConversionError.h
