#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "src/model/Document.h"

#include <QMainWindow>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class DocumentController;
class MarkdownDocumentRenderer;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void renderMarkdownRequested(quint64 requestId,
                                 const QString &markdown,
                                 const QString &htmlFilePath);

private slots:
    void openFile();
    void convertFile();
    void onConversionStarted();
    void onConversionFinished(const QString &markdown);
    void onConversionFailed(const QString &error);
    void onMarkdownRendered(quint64 requestId, const QString &htmlFilePath);
    void onMarkdownRenderFailed(quint64 requestId, const QString &error);
    void insertNextEditorChunk();
    void onPreviewLoadFinished(bool success);

private:
    bool isDocumentBusy() const;
    void replaceEditorDocument();
    void startEditorInsertion(const QString &markdown);
    void finishEditorInsertion();
    void startPreviewLoadIfReady();
    void failRendering(quint64 requestId, const QString &error);
    void invalidateRenderRequest();
    void updateDocumentPresentation();

    Ui::MainWindow *ui;
    DocumentController *m_controller;
    MarkdownDocumentRenderer *m_renderer;
    QTimer *m_editorChunkTimer;
    QThread m_rendererThread;
    QTemporaryDir m_renderDirectory;
    Document m_document;
    QString m_pendingEditorText;
    qsizetype m_editorTextOffset = 0;
    quint64 m_lastRenderRequestId = 0;
    quint64 m_activeRenderRequestId = 0;
    quint64 m_previewLoadRequestId = 0;
    QString m_renderedHtmlFilePath;
    QUrl m_expectedPreviewUrl;
    bool m_editorInsertionActive = false;
    bool m_editorInsertionFinished = false;
    bool m_htmlRenderingFinished = false;
    bool m_previewLoadPending = false;
};
#endif // MAINWINDOW_H
