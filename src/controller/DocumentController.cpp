#include "DocumentController.h"

#include "../markitdown/MarkItDownManager.h"

DocumentController::DocumentController(QObject *parent)
    : QObject(parent)
    , m_markItDown(new MarkItDownManager(this))
{
    connect(m_markItDown,
            &MarkItDownManager::started,
            this,
            &DocumentController::conversionStarted);
    connect(m_markItDown,
            &MarkItDownManager::finished,
            this,
            &DocumentController::conversionFinished);
    connect(m_markItDown,
            &MarkItDownManager::failed,
            this,
            &DocumentController::conversionFailed);
}

void DocumentController::convert(const QString &filePath)
{
    m_markItDown->convert(filePath);
}

bool DocumentController::isConverting() const
{
    return m_markItDown->isRunning();
}
