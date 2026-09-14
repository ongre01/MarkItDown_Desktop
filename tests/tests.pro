TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    unit \
    component \
    ui

unit.file = $$PWD/unit/unit.pro

component.file = $$PWD/component/component.pro

ui.file = $$PWD/ui/ui.pro

DISTFILES += $$PWD/run-tests.ps1
