QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/controller/DocumentController.cpp \
    src/markitdown/MarkItDownManager.cpp \
    src/rendering/MarkdownDocumentRenderer.cpp

HEADERS += \
    mainwindow.h \
    src/controller/DocumentController.h \
    src/markitdown/MarkItDownManager.h \
    src/model/Document.h \
    src/rendering/MarkdownDocumentRenderer.h

FORMS += \
    mainwindow.ui

DISTFILES += \
    requirements-markitdown.txt \
    scripts/install_markitdown_backend.ps1

win32 {
    markitdownInstaller = $$shell_path($$PWD/scripts/install_markitdown_backend.ps1)
    markitdownRequirements = $$shell_path($$PWD/requirements-markitdown.txt)

    CONFIG(debug, debug|release) {
        markitdownEnvironment = $$shell_path($$OUT_PWD/debug/python-venv)
    } else {
        markitdownEnvironment = $$shell_path($$OUT_PWD/release/python-venv)
    }

    QMAKE_POST_LINK += powershell.exe -NoProfile -ExecutionPolicy Bypass \
        -File $$shell_quote($$markitdownInstaller) \
        -Destination $$shell_quote($$markitdownEnvironment) \
        -RequirementsFile $$shell_quote($$markitdownRequirements)
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
