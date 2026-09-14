#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "src/model/ConversionError.h"
#include "src/model/Document.h"
#include "src/rendering/MarkdownRenderState.h"

#include <QMainWindow>
#include <QThread>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class DocumentController;
class MarkdownDocumentRenderer;
class QDragEnterEvent;
class QDropEvent;
class QTimer;

namespace DocumentFileOperations {
struct SourceDocumentValidation;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void renderMarkdownRequested(quint64 requestId,
                                 const QString &markdown);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openFile();
    void convertFile();
    void saveMarkdown();
    void saveMarkdownAs();
    void onConversionStarted();
    void onConversionFinished(const QString &markdown);
    void onConversionFailed(ConversionError error, const QString &details);
    void onMarkdownRendered(quint64 requestId, const QString &html);
    void onMarkdownEditorTextChanged();
    void renderEditorPreview();
    void insertNextEditorChunk();

private:
    bool isDocumentBusy() const;
    bool canSaveMarkdown() const;
    bool openDocument(const QString &filePath);
    bool saveMarkdownToFile(const QString &filePath);
    void showSourceDocumentError(
        const DocumentFileOperations::SourceDocumentValidation &validation);
    void showMarkdownSaveError(const QString &filePath, const QString &error);
    void replaceEditorDocument();
    void startEditorInsertion(const QString &markdown);
    void finishEditorInsertion();
    bool stopEditorInsertion();
    void finishInitialPreviewIfReady();
    void invalidateRenderRequest();
    void setDocumentStatus(DocumentStatus status);
    void updateUiState();

    std::unique_ptr<Ui::MainWindow> ui;
    DocumentController *const m_controller;
    MarkdownDocumentRenderer *const m_renderer;
    QTimer *const m_editorChunkTimer;
    QTimer *const m_previewUpdateTimer;
    QThread m_rendererThread;
    Document m_document;
    MarkdownRenderState m_renderState;
    QString m_pendingEditorText;
    qsizetype m_editorTextOffset = 0;
    bool m_editorInsertionActive = false;
};
#endif // MAINWINDOW_H
