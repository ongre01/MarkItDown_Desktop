TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    markitdown_manager \
    document_controller

markitdown_manager.file = $$PWD/tst_MarkItDownManager.pro
markitdown_manager.makefile = Makefile.MarkItDownManager

document_controller.file = $$PWD/tst_DocumentController.pro
document_controller.makefile = Makefile.DocumentController
