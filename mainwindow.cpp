#include "mainwindow.h"
#include "src/controller/DocumentController.h"
#include "src/rendering/MarkdownDocumentRenderer.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QMessageBox>
#include <QPlainTextDocumentLayout>
#include <QSaveFile>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

namespace {

constexpr qsizetype EditorChunkSize = 32 * 1024;
constexpr qsizetype MaximumEditorChunkSize = 64 * 1024;
constexpr int EditorChunkDelayMilliseconds = 1;
constexpr int PreviewUpdateDelayMilliseconds = 150;

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(new DocumentController(this))
    , m_renderer(new MarkdownDocumentRenderer)
    , m_editorChunkTimer(new QTimer(this))
    , m_previewUpdateTimer(new QTimer(this))
{
    ui->setupUi(this);
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
            &MainWindow::onMarkdownRendered);
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

    updateDocumentPresentation();
}

MainWindow::~MainWindow()
{
    invalidateRenderRequest();
    disconnect(m_renderer, nullptr, this, nullptr);
    m_rendererThread.quit();
    m_rendererThread.wait();
    delete ui;
}

void MainWindow::openFile()
{
    if (isDocumentBusy()) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Document"),
        QString(),
        tr("Documents (*.pdf *.docx *.pptx *.xlsx *.xls *.html *.htm *.csv *.json *.xml *.txt)"));

    if (filePath.isEmpty()) {
        return;
    }

    invalidateRenderRequest();

    m_document = Document{};
    m_document.sourceFilePath = QFileInfo(filePath).absoluteFilePath();
    m_document.status = DocumentStatus::Ready;

    replaceEditorDocument();
    ui->markdownPreview->clear();

    updateDocumentPresentation();
}

void MainWindow::convertFile()
{
    if (m_document.sourceFilePath.isEmpty() || isDocumentBusy()) {
        return;
    }

    invalidateRenderRequest();

    // Disable conversion-sensitive actions immediately so a second request cannot
    // be queued while QProcess is transitioning to its Starting state.
    m_document.status = DocumentStatus::Converting;
    updateDocumentPresentation();

    m_controller->convert(m_document.sourceFilePath);
}

void MainWindow::saveMarkdown()
{
    if (m_document.status != DocumentStatus::Completed || isDocumentBusy()) {
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
    if (m_document.status != DocumentStatus::Completed || isDocumentBusy()) {
        return;
    }

    QString suggestedFilePath = m_document.markdownFilePath;
    if (suggestedFilePath.isEmpty()) {
        const QFileInfo sourceInfo(m_document.sourceFilePath);
        suggestedFilePath = sourceInfo.dir().filePath(
            sourceInfo.completeBaseName() + QStringLiteral(".md"));
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Markdown As"),
        suggestedFilePath,
        tr("Markdown Files (*.md);;All Files (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (QFileInfo(filePath).suffix().isEmpty()) {
        filePath += QStringLiteral(".md");
    }

    saveMarkdownToFile(QFileInfo(filePath).absoluteFilePath());
}

void MainWindow::onConversionStarted()
{
    m_document.status = DocumentStatus::Converting;
    updateDocumentPresentation();
}

void MainWindow::onConversionFinished(const QString &markdown)
{
    invalidateRenderRequest();

    m_document.markdown = markdown;
    m_document.modified = false;
    m_document.status = DocumentStatus::Rendering;

    const quint64 requestId = ++m_lastRenderRequestId;
    m_activeRenderRequestId = requestId;
    m_renderedHtml.clear();
    m_editorInsertionFinished = false;
    m_htmlRenderingFinished = false;
    m_initialPreviewRendering = true;

    replaceEditorDocument();
    startEditorInsertion(markdown);

    updateDocumentPresentation();
    emit renderMarkdownRequested(requestId, markdown);
}

void MainWindow::onConversionFailed(const QString &error)
{
    invalidateRenderRequest();

    const QString message = error.trimmed().isEmpty()
                                ? tr("MarkItDown conversion failed.")
                                : error.trimmed();

    m_document.status = DocumentStatus::Failed;
    updateDocumentPresentation();

    const QString fileName = QFileInfo(m_document.sourceFilePath).fileName();
    ui->statusbar->showMessage(tr("%1 | Conversion Failed: %2").arg(fileName, message));
    QMessageBox::critical(this, tr("Conversion Failed"), message);
}

void MainWindow::onMarkdownRendered(quint64 requestId, const QString &html)
{
    if (requestId != m_activeRenderRequestId) {
        return;
    }

    if (m_initialPreviewRendering) {
        m_renderedHtml = html;
        m_htmlRenderingFinished = true;
        finishInitialPreviewIfReady();
        return;
    }

    m_activeRenderRequestId = 0;
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
    m_activeRenderRequestId = 0;
    m_previewUpdateTimer->start();
    updateDocumentPresentation();
}

void MainWindow::renderEditorPreview()
{
    if (m_document.status != DocumentStatus::Completed) {
        return;
    }

    const quint64 requestId = ++m_lastRenderRequestId;
    m_activeRenderRequestId = requestId;
    m_initialPreviewRendering = false;
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

bool MainWindow::saveMarkdownToFile(const QString &filePath)
{
    QSaveFile outputFile(filePath);
    const auto showSaveError = [this, &filePath](const QString &error) {
        QMessageBox::critical(
            this,
            tr("Save Failed"),
            tr("Could not save Markdown to:\n%1\n\n%2")
                .arg(QDir::toNativeSeparators(filePath), error));
    };

    if (!outputFile.open(QIODevice::WriteOnly)) {
        showSaveError(outputFile.errorString());
        return false;
    }

    const QByteArray utf8 = m_document.markdown.toUtf8();
    if (outputFile.write(utf8) != utf8.size()) {
        const QString error = outputFile.errorString();
        outputFile.cancelWriting();
        showSaveError(error);
        return false;
    }

    if (!outputFile.commit()) {
        showSaveError(outputFile.errorString());
        return false;
    }

    m_document.markdownFilePath = QFileInfo(filePath).absoluteFilePath();
    m_document.modified = false;
    ui->markdownEditor->document()->setModified(false);
    updateDocumentPresentation();
    ui->statusbar->showMessage(
        tr("Saved: %1").arg(QFileInfo(m_document.markdownFilePath).fileName()));

    return true;
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
    if (!m_editorInsertionActive) {
        return;
    }

    m_editorChunkTimer->stop();
    m_editorInsertionActive = false;
    m_editorInsertionFinished = true;
    m_pendingEditorText.clear();
    m_editorTextOffset = 0;

    ui->markdownEditor->document()->setModified(false);
    ui->markdownEditor->setUndoRedoEnabled(true);
    ui->markdownEditor->setReadOnly(false);
    ui->markdownEditor->setUpdatesEnabled(true);
    ui->markdownEditor->viewport()->update();

    finishInitialPreviewIfReady();
}

void MainWindow::finishInitialPreviewIfReady()
{
    if (m_activeRenderRequestId == 0
        || !m_initialPreviewRendering
        || !m_editorInsertionFinished
        || !m_htmlRenderingFinished) {
        return;
    }

    ui->markdownPreview->setHtml(m_renderedHtml);
    m_activeRenderRequestId = 0;
    m_initialPreviewRendering = false;
    m_renderedHtml.clear();
    m_document.status = DocumentStatus::Completed;
    m_document.modified = false;
    ui->markdownEditor->document()->setModified(false);
    updateDocumentPresentation();
}

void MainWindow::invalidateRenderRequest()
{
    m_previewUpdateTimer->stop();
    m_editorChunkTimer->stop();
    m_activeRenderRequestId = 0;
    m_initialPreviewRendering = false;
    finishEditorInsertion();
    m_htmlRenderingFinished = false;
    m_editorInsertionFinished = false;
    m_renderedHtml.clear();
}

void MainWindow::updateDocumentPresentation()
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

    const bool isBusy = m_document.status == DocumentStatus::Converting
                        || m_document.status == DocumentStatus::Rendering;
    const bool hasDocument = !m_document.sourceFilePath.isEmpty();
    const bool hasMarkdown = m_document.status == DocumentStatus::Completed;

    ui->actionOpen->setEnabled(!isBusy);
    ui->actionConvert->setEnabled(hasDocument && !isBusy);
    ui->actionSave->setEnabled(hasMarkdown && !isBusy);
    ui->actionSaveAs->setEnabled(hasMarkdown && !isBusy);
}
