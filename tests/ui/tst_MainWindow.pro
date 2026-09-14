QT = core gui widgets testlib

TARGET = tst_MainWindow

include(ui_test.pri)

SOURCES += \
    tst_MainWindow.cpp \
    ../../mainwindow.cpp \
    ../../src/controller/DocumentController.cpp \
    ../../src/io/DocumentFileOperations.cpp \
    ../../src/markitdown/MarkItDownExecutableResolver.cpp \
    ../../src/markitdown/MarkItDownManager.cpp \
    ../../src/markitdown/ProcessRunner.cpp \
    ../../src/rendering/MarkdownDocumentRenderer.cpp \
    ../../src/rendering/MarkdownRenderState.cpp \
    ../../src/ui/ConversionErrorPresentation.cpp

HEADERS += \
    ../../mainwindow.h \
    ../../src/controller/DocumentController.h \
    ../../src/io/DocumentFileOperations.h \
    ../../src/markitdown/IMarkItDownManager.h \
    ../../src/markitdown/MarkItDownExecutableResolver.h \
    ../../src/markitdown/MarkItDownManager.h \
    ../../src/markitdown/ProcessRunner.h \
    ../../src/model/ConversionError.h \
    ../../src/model/Document.h \
    ../../src/rendering/MarkdownDocumentRenderer.h \
    ../../src/rendering/MarkdownRenderState.h \
    ../../src/ui/ConversionErrorPresentation.h \
    ../../src/ui/IMainWindowDialogs.h

FORMS += \
    ../../mainwindow.ui
