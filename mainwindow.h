#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "src/model/Document.h"

#include <QMainWindow>
#include <QThread>

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
                                 const QString &markdown);

private slots:
    void openFile();
    void convertFile();
    void onConversionStarted();
    void onConversionFinished(const QString &markdown);
    void onConversionFailed(const QString &error);
    void onMarkdownRendered(quint64 requestId, const QString &html);
    void onMarkdownEditorTextChanged();
    void renderEditorPreview();
    void insertNextEditorChunk();

private:
    bool isDocumentBusy() const;
    void replaceEditorDocument();
    void startEditorInsertion(const QString &markdown);
    void finishEditorInsertion();
    void finishInitialPreviewIfReady();
    void invalidateRenderRequest();
    void updateDocumentPresentation();

    Ui::MainWindow *ui;
    DocumentController *m_controller;
    MarkdownDocumentRenderer *m_renderer;
    QTimer *m_editorChunkTimer;
    QTimer *m_previewUpdateTimer;
    QThread m_rendererThread;
    Document m_document;
    QString m_pendingEditorText;
    qsizetype m_editorTextOffset = 0;
    quint64 m_lastRenderRequestId = 0;
    quint64 m_activeRenderRequestId = 0;
    QString m_renderedHtml;
    bool m_editorInsertionActive = false;
    bool m_editorInsertionFinished = false;
    bool m_htmlRenderingFinished = false;
    bool m_initialPreviewRendering = false;
};
#endif // MAINWINDOW_H
