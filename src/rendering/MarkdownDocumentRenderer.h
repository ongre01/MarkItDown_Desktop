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
                const QString &markdown);

signals:
    void rendered(quint64 requestId, const QString &html);
};

#endif // MARKDOWNDOCUMENTRENDERER_H
