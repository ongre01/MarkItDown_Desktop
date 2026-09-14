QT = core testlib

TARGET = tst_MarkdownRenderState

include(unit_test.pri)

SOURCES += \
    tst_MarkdownRenderState.cpp \
    ../../src/rendering/MarkdownRenderState.cpp

HEADERS += \
    ../../src/rendering/MarkdownRenderState.h
