TEMPLATE = app

CONFIG += console testcase c++17
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/../..

DESTDIR = $$OUT_PWD/bin
OBJECTS_DIR = $$OUT_PWD/.obj/$${TARGET}
MOC_DIR = $$OUT_PWD/.moc/$${TARGET}
RCC_DIR = $$OUT_PWD/.rcc/$${TARGET}
UI_DIR = $$OUT_PWD/.uic/$${TARGET}
