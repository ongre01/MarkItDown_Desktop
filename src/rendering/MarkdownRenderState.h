#ifndef MARKDOWNRENDERSTATE_H
#define MARKDOWNRENDERSTATE_H

#include <QString>
#include <QtGlobal>

class MarkdownRenderState
{
public:
    quint64 beginInitialRequest();
    quint64 beginUpdateRequest();

    bool accepts(quint64 requestId) const noexcept;
    bool isInitialRequest() const noexcept;
    bool isInitialRequestReady() const noexcept;

    void storeRenderedHtml(const QString &html);
    const QString &renderedHtml() const noexcept;
    void markEditorInsertionFinished() noexcept;

    void completeRequest();
    void invalidate();

private:
    quint64 nextRequestId();

    quint64 m_lastRequestId = 0;
    quint64 m_activeRequestId = 0;
    QString m_renderedHtml;
    bool m_editorInsertionFinished = false;
    bool m_htmlRenderingFinished = false;
    bool m_initialRequest = false;
};

#endif // MARKDOWNRENDERSTATE_H
