#include "DocumentController.h"

#include "../markitdown/IMarkItDownManager.h"
#include "../markitdown/MarkItDownManager.h"

#include <utility>

DocumentController::DocumentController(QObject *parent)
    : DocumentController(std::make_unique<MarkItDownManager>(), parent)
{
}

DocumentController::DocumentController(
    std::unique_ptr<IMarkItDownManager> markItDown,
    QObject *parent)
    : QObject(parent)
    , m_markItDown(std::move(markItDown))
{
    Q_ASSERT(m_markItDown);

    connect(m_markItDown.get(),
            &IMarkItDownManager::started,
            this,
            &DocumentController::conversionStarted);
    connect(m_markItDown.get(),
            &IMarkItDownManager::finished,
            this,
            &DocumentController::conversionFinished);
    connect(m_markItDown.get(),
            &IMarkItDownManager::failed,
            this,
            &DocumentController::conversionFailed);
}

DocumentController::~DocumentController() = default;

void DocumentController::convert(const QString &filePath)
{
    m_markItDown->convert(filePath);
}

bool DocumentController::isConverting() const
{
    return m_markItDown->isRunning();
}
