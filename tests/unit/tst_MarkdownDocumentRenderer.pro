QT = core gui testlib

TARGET = tst_MarkdownDocumentRenderer

include(unit_test.pri)

SOURCES += \
    tst_MarkdownDocumentRenderer.cpp \
    ../../src/rendering/MarkdownDocumentRenderer.cpp

HEADERS += \
    ../../src/rendering/MarkdownDocumentRenderer.h
