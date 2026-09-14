#include "MarkdownRenderState.h"

quint64 MarkdownRenderState::beginInitialRequest()
{
    m_activeRequestId = nextRequestId();
    m_renderedHtml.clear();
    m_editorInsertionFinished = false;
    m_htmlRenderingFinished = false;
    m_initialRequest = true;
    return m_activeRequestId;
}

quint64 MarkdownRenderState::beginUpdateRequest()
{
    m_activeRequestId = nextRequestId();
    m_renderedHtml.clear();
    m_editorInsertionFinished = false;
    m_htmlRenderingFinished = false;
    m_initialRequest = false;
    return m_activeRequestId;
}

bool MarkdownRenderState::accepts(quint64 requestId) const noexcept
{
    return m_activeRequestId != 0 && requestId == m_activeRequestId;
}

bool MarkdownRenderState::isInitialRequest() const noexcept
{
    return m_initialRequest;
}

bool MarkdownRenderState::isInitialRequestReady() const noexcept
{
    return m_activeRequestId != 0
           && m_initialRequest
           && m_editorInsertionFinished
           && m_htmlRenderingFinished;
}

void MarkdownRenderState::storeRenderedHtml(const QString &html)
{
    m_renderedHtml = html;
    m_htmlRenderingFinished = true;
}

const QString &MarkdownRenderState::renderedHtml() const noexcept
{
    return m_renderedHtml;
}

void MarkdownRenderState::markEditorInsertionFinished() noexcept
{
    m_editorInsertionFinished = true;
}

void MarkdownRenderState::completeRequest()
{
    invalidate();
}

void MarkdownRenderState::invalidate()
{
    m_activeRequestId = 0;
    m_renderedHtml.clear();
    m_editorInsertionFinished = false;
    m_htmlRenderingFinished = false;
    m_initialRequest = false;
}

quint64 MarkdownRenderState::nextRequestId()
{
    ++m_lastRequestId;
    if (m_lastRequestId == 0) {
        ++m_lastRequestId;
    }

    return m_lastRequestId;
}
