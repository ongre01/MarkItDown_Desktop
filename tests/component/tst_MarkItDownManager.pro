QT = core testlib

TARGET = tst_MarkItDownManager

include(component_test.pri)

SOURCES += \
    tst_MarkItDownManager.cpp \
    ../../src/markitdown/MarkItDownExecutableResolver.cpp \
    ../../src/markitdown/MarkItDownManager.cpp \
    ../../src/markitdown/ProcessRunner.cpp

HEADERS += \
    ../../src/markitdown/IMarkItDownManager.h \
    ../../src/markitdown/MarkItDownExecutableResolver.h \
    ../../src/markitdown/MarkItDownManager.h \
    ../../src/markitdown/ProcessRunner.h \
    ../../src/model/ConversionError.h
