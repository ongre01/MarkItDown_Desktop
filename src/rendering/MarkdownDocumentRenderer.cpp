#include "MarkdownDocumentRenderer.h"

#include <QSaveFile>
#include <QTextDocument>

MarkdownDocumentRenderer::MarkdownDocumentRenderer(QObject *parent)
    : QObject(parent)
{
}

void MarkdownDocumentRenderer::render(quint64 requestId,
                                      const QString &markdown,
                                      const QString &htmlFilePath)
{
    QTextDocument document;
    document.setMarkdown(markdown);

    QSaveFile htmlFile(htmlFilePath);
    if (!htmlFile.open(QIODevice::WriteOnly)) {
        emit failed(requestId,
                    tr("Could not create the preview file: %1").arg(htmlFile.errorString()));
        return;
    }

    const QByteArray html = document.toHtml().toUtf8();
    if (htmlFile.write(html) != html.size()) {
        emit failed(requestId,
                    tr("Could not write the preview file: %1").arg(htmlFile.errorString()));
        htmlFile.cancelWriting();
        return;
    }

    if (!htmlFile.commit()) {
        emit failed(requestId,
                    tr("Could not finish the preview file: %1").arg(htmlFile.errorString()));
        return;
    }

    emit rendered(requestId, htmlFilePath);
}
