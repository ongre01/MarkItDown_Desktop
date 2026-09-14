TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += main_window

main_window.file = $$PWD/tst_MainWindow.pro
main_window.makefile = Makefile.MainWindow
