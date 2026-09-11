#ifndef MARKDOWNDOCUMENTRENDERER_H
#define MARKDOWNDOCUMENTRENDERER_H

#include <QObject>
#include <QString>

class MarkdownDocumentRenderer : public QObject
{
    Q_OBJECT

public:
    explicit MarkdownDocumentRenderer(QObject *parent = nullptr);

public slots:
    void render(quint64 requestId,
                const QString &markdown,
                const QString &htmlFilePath);

signals:
    void rendered(quint64 requestId, const QString &htmlFilePath);
    void failed(quint64 requestId, const QString &error);
};

#endif // MARKDOWNDOCUMENTRENDERER_H
