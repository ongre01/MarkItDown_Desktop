TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    document_file_operations \
    markdown_render_state \
    conversion_error_presentation \
    markdown_document_renderer \
    markitdown_manager

document_file_operations.file = $$PWD/unit/tst_DocumentFileOperations.pro
document_file_operations.makefile = Makefile.DocumentFileOperations

markdown_render_state.file = $$PWD/unit/tst_MarkdownRenderState.pro
markdown_render_state.makefile = Makefile.MarkdownRenderState

conversion_error_presentation.file = $$PWD/unit/tst_ConversionErrorPresentation.pro
conversion_error_presentation.makefile = Makefile.ConversionErrorPresentation

markdown_document_renderer.file = $$PWD/unit/tst_MarkdownDocumentRenderer.pro
markdown_document_renderer.makefile = Makefile.MarkdownDocumentRenderer

markitdown_manager.file = $$PWD/component/tst_MarkItDownManager.pro
markitdown_manager.makefile = Makefile.MarkItDownManager
