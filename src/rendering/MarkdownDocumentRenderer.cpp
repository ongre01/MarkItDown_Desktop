#include "MarkdownDocumentRenderer.h"

#include <QTextDocument>

MarkdownDocumentRenderer::MarkdownDocumentRenderer(QObject *parent)
    : QObject(parent)
{
}

void MarkdownDocumentRenderer::render(quint64 requestId,
                                      const QString &markdown)
{
    QTextDocument document;
    document.setMarkdown(markdown);
    emit rendered(requestId, document.toHtml());
}
