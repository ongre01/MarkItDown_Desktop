#include "mainwindow.h"
#include "src/controller/DocumentController.h"
#include "src/io/DocumentFileOperations.h"
#include "src/rendering/MarkdownDocumentRenderer.h"
#include "src/ui/ConversionErrorPresentation.h"
#include "src/ui/IMainWindowDialogs.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextDocumentLayout>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

#include <utility>

namespace {

constexpr qsizetype EditorChunkSize = 32 * 1024;
constexpr qsizetype MaximumEditorChunkSize = 64 * 1024;
constexpr int EditorChunkDelayMilliseconds = 1;
constexpr int PreviewUpdateDelayMilliseconds = 150;

class NativeMainWindowDialogs final : public IMainWindowDialogs
{
public:
    QString selectSourceDocument(QWidget *parent,
                                 const QString &filter) override
    {
        return QFileDialog::getOpenFileName(
            parent,
            MainWindow::tr("Open Document"),
            QString(),
            filter);
    }

    QString selectMarkdownDestination(
        QWidget *parent,
        const QString &suggestedFilePath) override
    {
        return QFileDialog::getSaveFileName(
            parent,
            MainWindow::tr("Save Markdown As"),
            suggestedFilePath,
            MainWindow::tr("Markdown Files (*.md);;All Files (*.*)"));
    }

    void showWarning(QWidget *parent,
                     const QString &title,
                     const QString &message) override
    {
        QMessageBox::warning(parent, title, message);
    }

    void showCritical(QWidget *parent,
                      const QString &title,
                      const QString &message) override
    {
        QMessageBox::critical(parent, title, message);
    }
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : MainWindow(std::make_unique<DocumentController>(),
                 std::make_unique<MarkdownDocumentRenderer>(),
                 std::make_unique<NativeMainWindowDialogs>(),
                 parent)
{
}

MainWindow::MainWindow(std::unique_ptr<DocumentController> controller,
                       std::unique_ptr<MarkdownDocumentRenderer> renderer,
                       std::unique_ptr<IMainWindowDialogs> dialogs,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , m_controller(controller.release())
    , m_renderer(renderer.release())
    , m_dialogs(std::move(dialogs))
    , m_editorChunkTimer(new QTimer(this))
    , m_previewUpdateTimer(new QTimer(this))
{
    Q_ASSERT(m_controller);
    Q_ASSERT(m_renderer);
    Q_ASSERT(m_dialogs);
    Q_ASSERT(!m_controller->parent());
    Q_ASSERT(!m_renderer->parent());

    m_controller->setParent(this);

    ui->setupUi(this);
    setAcceptDrops(true);
    ui->markdownEditor->setAcceptDrops(false);
    ui->markdownPreview->setAcceptDrops(false);
    ui->contentSplitter->setStretchFactor(0, 1);
    ui->contentSplitter->setStretchFactor(1, 1);
    QTimer::singleShot(0, this, [this]() {
        const int splitterWidth = ui->contentSplitter->width();
        ui->contentSplitter->setSizes(
            QList<int>{splitterWidth / 2, splitterWidth - splitterWidth / 2});
    });

    m_renderer->moveToThread(&m_rendererThread);
    connect(&m_rendererThread,
            &QThread::finished,
            m_renderer,
            &QObject::deleteLater);
    connect(this,
            &MainWindow::renderMarkdownRequested,
            m_renderer,
            &MarkdownDocumentRenderer::render,
            Qt::QueuedConnection);
    connect(m_renderer,
            &MarkdownDocumentRenderer::rendered,
            this,
            &MainWindow::onMarkdownRendered,
            Qt::QueuedConnection);
    m_rendererThread.start();

    m_editorChunkTimer->setSingleShot(true);
    connect(m_editorChunkTimer,
            &QTimer::timeout,
            this,
            &MainWindow::insertNextEditorChunk);

    m_previewUpdateTimer->setSingleShot(true);
    m_previewUpdateTimer->setInterval(PreviewUpdateDelayMilliseconds);
    connect(m_previewUpdateTimer,
            &QTimer::timeout,
            this,
            &MainWindow::renderEditorPreview);

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionConvert, &QAction::triggered, this, &MainWindow::convertFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveMarkdown);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveMarkdownAs);
    connect(m_controller,
            &DocumentController::conversionStarted,
            this,
            &MainWindow::onConversionStarted);
    connect(m_controller,
            &DocumentController::conversionFinished,
            this,
            &MainWindow::onConversionFinished);
    connect(m_controller,
            &DocumentController::conversionFailed,
            this,
            &MainWindow::onConversionFailed);
    connect(ui->markdownEditor,
            &QPlainTextEdit::textChanged,
            this,
            &MainWindow::onMarkdownEditorTextChanged);

    updateUiState();
}

MainWindow::~MainWindow()
{
    invalidateRenderRequest();
    disconnect(m_renderer, nullptr, this, nullptr);
    disconnect(this, nullptr, m_renderer, nullptr);
    m_rendererThread.quit();
    m_rendererThread.wait();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!isDocumentBusy() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (isDocumentBusy() || !event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.size() != 1) {
        m_dialogs->showWarning(
            this,
            tr("Open Document"),
            tr(u8"한 번에 하나의 파일만 열 수 있습니다."));
        event->ignore();
        return;
    }

    const QUrl &url = urls.constFirst();
    if (!url.isLocalFile()) {
        m_dialogs->showWarning(
            this,
            tr("Open Document"),
            tr(u8"로컬 파일만 열 수 있습니다."));
        event->ignore();
        return;
    }

    if (openDocument(url.toLocalFile())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::openFile()
{
    if (isDocumentBusy()) {
        return;
    }

    const QString filePath = m_dialogs->selectSourceDocument(
        this,
        tr("Documents (%1)")
            .arg(DocumentFileOperations::supportedNameFilters().join(QLatin1Char(' '))));

    if (filePath.isEmpty()) {
        return;
    }

    openDocument(filePath);
}

bool MainWindow::openDocument(const QString &filePath)
{
    if (isDocumentBusy()) {
        return false;
    }

    const DocumentFileOperations::SourceDocumentValidation validation =
        DocumentFileOperations::validateSourceDocument(filePath);
    if (!validation.isValid()) {
        showSourceDocumentError(validation);
        return false;
    }

    invalidateRenderRequest();

    m_document = Document{};
    m_document.sourceFilePath = validation.absoluteFilePath;
    m_document.status = DocumentStatus::Ready;

    replaceEditorDocument();
    ui->markdownPreview->clear();

    updateUiState();
    convertFile();
    return true;
}

void MainWindow::convertFile()
{
    if (m_document.sourceFilePath.isEmpty() || isDocumentBusy()) {
        return;
    }

    const DocumentFileOperations::SourceDocumentValidation validation =
        DocumentFileOperations::validateSourceDocument(m_document.sourceFilePath);
    if (!validation.isValid()) {
        setDocumentStatus(DocumentStatus::Failed);
        ui->statusbar->showMessage(
            tr("%1 | Source File Not Found")
                .arg(QFileInfo(m_document.sourceFilePath).fileName()));
        showSourceDocumentError(validation);
        return;
    }

    invalidateRenderRequest();

    // Disable conversion-sensitive actions immediately so a second request cannot
    // be queued while QProcess is transitioning to its Starting state.
    setDocumentStatus(DocumentStatus::Converting);

    m_controller->convert(m_document.sourceFilePath);
}

void MainWindow::saveMarkdown()
{
    if (!canSaveMarkdown()) {
        return;
    }

    if (m_document.markdownFilePath.isEmpty()) {
        saveMarkdownAs();
        return;
    }

    saveMarkdownToFile(m_document.markdownFilePath);
}

void MainWindow::saveMarkdownAs()
{
    if (!canSaveMarkdown()) {
        return;
    }

    QString suggestedFilePath = m_document.markdownFilePath;
    if (suggestedFilePath.isEmpty()) {
        suggestedFilePath =
            DocumentFileOperations::suggestedMarkdownPath(m_document.sourceFilePath);
    }

    QString filePath =
        m_dialogs->selectMarkdownDestination(this, suggestedFilePath);

    if (filePath.isEmpty()) {
        return;
    }

    saveMarkdownToFile(DocumentFileOperations::normalizedMarkdownPath(filePath));
}

void MainWindow::onConversionStarted()
{
    setDocumentStatus(DocumentStatus::Converting);
}

void MainWindow::onConversionFinished(const QString &markdown)
{
    invalidateRenderRequest();

    m_document.markdown = markdown;
    m_document.modified = false;
    m_document.status = DocumentStatus::Rendering;

    const quint64 requestId = m_renderState.beginInitialRequest();

    replaceEditorDocument();
    startEditorInsertion(markdown);

    updateUiState();
    emit renderMarkdownRequested(requestId, markdown);
}

void MainWindow::onConversionFailed(ConversionError error, const QString &details)
{
    invalidateRenderRequest();

    const ConversionErrorPresentation presentation = conversionErrorPresentation(error);
    QString message = presentation.summary;
    const QString normalizedDetails = details.trimmed();
    if (!normalizedDetails.isEmpty()) {
        message += tr("\n\nTechnical details:\n%1").arg(normalizedDetails);
    }

    setDocumentStatus(DocumentStatus::Failed);

    const QString fileName = QFileInfo(m_document.sourceFilePath).fileName();
    ui->statusbar->showMessage(
        tr("%1 | Conversion Failed: %2").arg(fileName, presentation.summary));
    m_dialogs->showCritical(this, presentation.title, message);
}

void MainWindow::onMarkdownRendered(quint64 requestId, const QString &html)
{
    if (!m_renderState.accepts(requestId)) {
        return;
    }

    if (m_renderState.isInitialRequest()) {
        m_renderState.storeRenderedHtml(html);
        finishInitialPreviewIfReady();
        return;
    }

    m_renderState.completeRequest();
    if (m_document.status != DocumentStatus::Converting) {
        ui->markdownPreview->setHtml(html);
    }
}

void MainWindow::onMarkdownEditorTextChanged()
{
    if (m_editorInsertionActive || m_document.status != DocumentStatus::Completed) {
        return;
    }

    m_document.markdown = ui->markdownEditor->toPlainText();
    m_document.modified = true;
    m_renderState.invalidate();
    m_previewUpdateTimer->start();
    updateUiState();
}

void MainWindow::renderEditorPreview()
{
    if (m_document.status != DocumentStatus::Completed) {
        return;
    }

    const quint64 requestId = m_renderState.beginUpdateRequest();
    emit renderMarkdownRequested(requestId, m_document.markdown);
}

void MainWindow::insertNextEditorChunk()
{
    if (!m_editorInsertionActive || m_document.status != DocumentStatus::Rendering) {
        return;
    }

    if (m_editorTextOffset >= m_pendingEditorText.size()) {
        finishEditorInsertion();
        return;
    }

    const qsizetype textSize = m_pendingEditorText.size();
    const qsizetype idealEnd = qMin(m_editorTextOffset + EditorChunkSize, textSize);
    const qsizetype maximumEnd = qMin(m_editorTextOffset + MaximumEditorChunkSize, textSize);
    qsizetype chunkEnd = idealEnd;

    if (idealEnd < textSize) {
        const qsizetype followingLineEnd = m_pendingEditorText.indexOf(QLatin1Char('\n'), idealEnd);
        if (followingLineEnd >= 0 && followingLineEnd < maximumEnd) {
            chunkEnd = followingLineEnd + 1;
        } else {
            const qsizetype precedingLineEnd =
                m_pendingEditorText.lastIndexOf(QLatin1Char('\n'), idealEnd - 1);
            if (precedingLineEnd >= m_editorTextOffset) {
                chunkEnd = precedingLineEnd + 1;
            } else {
                chunkEnd = maximumEnd;
            }
        }
    }

    QTextCursor cursor(ui->markdownEditor->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(m_pendingEditorText.mid(m_editorTextOffset,
                                              chunkEnd - m_editorTextOffset));
    m_editorTextOffset = chunkEnd;

    if (m_editorTextOffset >= textSize) {
        finishEditorInsertion();
    } else {
        m_editorChunkTimer->start(EditorChunkDelayMilliseconds);
    }
}

bool MainWindow::isDocumentBusy() const
{
    return m_controller->isConverting()
           || m_document.status == DocumentStatus::Converting
           || m_document.status == DocumentStatus::Rendering;
}

bool MainWindow::canSaveMarkdown() const
{
    return m_document.status == DocumentStatus::Completed && !isDocumentBusy();
}

bool MainWindow::saveMarkdownToFile(const QString &filePath)
{
    const DocumentFileOperations::MarkdownWriteResult result =
        DocumentFileOperations::writeMarkdownUtf8(filePath, m_document.markdown);
    if (!result.succeeded) {
        showMarkdownSaveError(result.absoluteFilePath, result.errorMessage);
        return false;
    }

    m_document.markdownFilePath = result.absoluteFilePath;
    m_document.modified = false;
    ui->markdownEditor->document()->setModified(false);
    updateUiState();
    ui->statusbar->showMessage(
        tr("Saved: %1").arg(QFileInfo(m_document.markdownFilePath).fileName()));

    return true;
}

void MainWindow::showSourceDocumentError(
    const DocumentFileOperations::SourceDocumentValidation &validation)
{
    switch (validation.error) {
    case DocumentFileOperations::SourceDocumentError::Directory:
        m_dialogs->showWarning(
            this,
            tr("Open Document"),
            tr(u8"폴더는 열 수 없습니다."));
        return;
    case DocumentFileOperations::SourceDocumentError::NotFound:
        m_dialogs->showWarning(
            this,
            tr("Source File Not Found"),
            tr(u8"원본 파일을 찾을 수 없습니다.\n\n%1")
                .arg(QDir::toNativeSeparators(validation.absoluteFilePath)));
        return;
    case DocumentFileOperations::SourceDocumentError::NotFile:
        m_dialogs->showWarning(
            this,
            tr("Cannot Open Source File"),
            tr(u8"원본 파일을 열 수 없습니다.\n\n%1")
                .arg(QDir::toNativeSeparators(validation.absoluteFilePath)));
        return;
    case DocumentFileOperations::SourceDocumentError::UnsupportedExtension: {
        const QString extension = validation.extension.isEmpty()
                                      ? tr(u8"확장자 없음")
                                      : QStringLiteral(".%1").arg(validation.extension);
        m_dialogs->showWarning(
            this,
            tr("Unsupported File Type"),
            tr(u8"지원하지 않는 파일 형식입니다.\n\n확장자: %1")
                .arg(extension));
        return;
    }
    case DocumentFileOperations::SourceDocumentError::None:
        return;
    }
}

void MainWindow::showMarkdownSaveError(const QString &filePath, const QString &error)
{
    const QString details = error.trimmed().isEmpty()
                                ? tr(u8"알 수 없는 파일 시스템 오류입니다.")
                                : error.trimmed();
    ui->statusbar->showMessage(
        tr("Markdown Save Failed: %1").arg(details));
    m_dialogs->showCritical(
        this,
        tr("Markdown Save Failed"),
        tr(u8"Markdown 파일을 저장하지 못했습니다.\n\n경로:\n%1\n\n기술 세부 정보:\n%2")
            .arg(QDir::toNativeSeparators(filePath), details));
}

void MainWindow::replaceEditorDocument()
{
    QTextDocument *oldDocument = ui->markdownEditor->document();
    if (oldDocument) {
        // QPlainTextEdit::setDocument() deletes its current document when the
        // editor owns it. Detach ownership first so deleteLater() remains valid.
        oldDocument->setParent(nullptr);
    }

    auto *newDocument = new QTextDocument(ui->markdownEditor);
    newDocument->setDefaultFont(ui->markdownEditor->font());
    newDocument->setDocumentLayout(new QPlainTextDocumentLayout(newDocument));
    ui->markdownEditor->setDocument(newDocument);

    if (oldDocument && oldDocument != newDocument) {
        oldDocument->deleteLater();
    }
}

void MainWindow::startEditorInsertion(const QString &markdown)
{
    m_pendingEditorText = markdown;
    m_editorTextOffset = 0;
    m_editorInsertionActive = true;

    ui->markdownEditor->setReadOnly(true);
    ui->markdownEditor->setUndoRedoEnabled(false);
    ui->markdownEditor->setUpdatesEnabled(false);

    if (m_pendingEditorText.isEmpty()) {
        finishEditorInsertion();
    } else {
        m_editorChunkTimer->start(0);
    }
}

void MainWindow::finishEditorInsertion()
{
    if (!stopEditorInsertion()) {
        return;
    }

    m_renderState.markEditorInsertionFinished();
    finishInitialPreviewIfReady();
}

bool MainWindow::stopEditorInsertion()
{
    m_editorChunkTimer->stop();

    if (!m_editorInsertionActive) {
        return false;
    }

    m_editorInsertionActive = false;
    m_pendingEditorText.clear();
    m_editorTextOffset = 0;

    ui->markdownEditor->document()->setModified(false);
    ui->markdownEditor->setUndoRedoEnabled(true);
    ui->markdownEditor->setReadOnly(false);
    ui->markdownEditor->setUpdatesEnabled(true);
    ui->markdownEditor->viewport()->update();

    return true;
}

void MainWindow::finishInitialPreviewIfReady()
{
    if (!m_renderState.isInitialRequestReady()) {
        return;
    }

    ui->markdownPreview->setHtml(m_renderState.renderedHtml());
    m_renderState.completeRequest();
    m_document.modified = false;
    ui->markdownEditor->document()->setModified(false);
    setDocumentStatus(DocumentStatus::Completed);
}

void MainWindow::invalidateRenderRequest()
{
    m_previewUpdateTimer->stop();
    stopEditorInsertion();
    m_renderState.invalidate();
}

void MainWindow::setDocumentStatus(DocumentStatus status)
{
    m_document.status = status;
    updateUiState();
}

void MainWindow::updateUiState()
{
    const QString fileName = QFileInfo(m_document.sourceFilePath).fileName();

    if (m_document.status == DocumentStatus::Empty) {
        setWindowTitle(tr("MarkItDown Viewer"));
        ui->statusbar->showMessage(tr("Ready"));
    } else {
        setWindowTitle(tr("MarkItDown Viewer - %1%2")
                           .arg(fileName, m_document.modified ? QStringLiteral(" *")
                                                             : QString()));

        switch (m_document.status) {
        case DocumentStatus::Ready:
            ui->statusbar->showMessage(tr("%1 | Ready").arg(fileName));
            break;
        case DocumentStatus::Converting:
            ui->statusbar->showMessage(tr("%1 | Converting...").arg(fileName));
            break;
        case DocumentStatus::Rendering:
            ui->statusbar->showMessage(tr("%1 | Rendering preview...").arg(fileName));
            break;
        case DocumentStatus::Completed:
            ui->statusbar->showMessage(tr("%1 | Converted | UTF-8 | Markdown").arg(fileName));
            break;
        case DocumentStatus::Failed:
            ui->statusbar->showMessage(tr("%1 | Conversion Failed").arg(fileName));
            break;
        case DocumentStatus::Empty:
            break;
        }
    }

    bool openEnabled = true;
    bool convertEnabled = false;
    bool saveEnabled = false;

    switch (m_document.status) {
    case DocumentStatus::Empty:
        break;
    case DocumentStatus::Ready:
    case DocumentStatus::Failed:
        convertEnabled = true;
        break;
    case DocumentStatus::Converting:
    case DocumentStatus::Rendering:
        openEnabled = false;
        break;
    case DocumentStatus::Completed:
        convertEnabled = true;
        saveEnabled = true;
        break;
    }

    ui->actionOpen->setEnabled(openEnabled);
    ui->actionConvert->setEnabled(convertEnabled);
    ui->actionSave->setEnabled(saveEnabled);
    ui->actionSaveAs->setEnabled(saveEnabled);
}
