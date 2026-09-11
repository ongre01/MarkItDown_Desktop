#include "mainwindow.h"
#include "src/controller/DocumentController.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(new DocumentController(this))
{
    ui->setupUi(this);

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionConvert, &QAction::triggered, this, &MainWindow::convertFile);
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

    updateDocumentPresentation();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Document"),
        QString(),
        tr("Documents (*.pdf *.docx *.pptx *.xlsx *.xls *.html *.htm *.csv *.json *.xml *.txt)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_document = Document{};
    m_document.sourceFilePath = QFileInfo(filePath).absoluteFilePath();
    m_document.status = DocumentStatus::Ready;

    ui->markdownEditor->clear();
    ui->markdownPreview->clear();

    updateDocumentPresentation();
}

void MainWindow::convertFile()
{
    if (m_document.sourceFilePath.isEmpty() || m_controller->isConverting()) {
        return;
    }

    // Disable conversion-sensitive actions immediately so a second request cannot
    // be queued while QProcess is transitioning to its Starting state.
    m_document.status = DocumentStatus::Converting;
    updateDocumentPresentation();

    m_controller->convert(m_document.sourceFilePath);
}

void MainWindow::onConversionStarted()
{
    m_document.status = DocumentStatus::Converting;
    updateDocumentPresentation();
}

void MainWindow::onConversionFinished(const QString &markdown)
{
    m_document.markdown = markdown;
    m_document.modified = false;
    m_document.status = DocumentStatus::Completed;

    ui->markdownEditor->setPlainText(markdown);
    ui->markdownPreview->setMarkdown(markdown);

    updateDocumentPresentation();
}

void MainWindow::onConversionFailed(const QString &error)
{
    const QString message = error.trimmed().isEmpty()
                                ? tr("MarkItDown conversion failed.")
                                : error.trimmed();

    m_document.status = DocumentStatus::Failed;
    updateDocumentPresentation();

    const QString fileName = QFileInfo(m_document.sourceFilePath).fileName();
    ui->statusbar->showMessage(tr("%1 | Conversion Failed: %2").arg(fileName, message));
    QMessageBox::critical(this, tr("Conversion Failed"), message);
}

void MainWindow::updateDocumentPresentation()
{
    const QString fileName = QFileInfo(m_document.sourceFilePath).fileName();

    if (m_document.status == DocumentStatus::Empty) {
        setWindowTitle(tr("MarkItDown Viewer"));
        ui->statusbar->showMessage(tr("Ready"));
    } else {
        setWindowTitle(tr("MarkItDown Viewer - %1").arg(fileName));

        switch (m_document.status) {
        case DocumentStatus::Ready:
            ui->statusbar->showMessage(tr("%1 | Ready").arg(fileName));
            break;
        case DocumentStatus::Converting:
            ui->statusbar->showMessage(tr("%1 | Converting...").arg(fileName));
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

    const bool isConverting = m_document.status == DocumentStatus::Converting;
    const bool hasDocument = !m_document.sourceFilePath.isEmpty();
    const bool hasMarkdown = m_document.status == DocumentStatus::Completed;

    ui->actionOpen->setEnabled(!isConverting);
    ui->actionConvert->setEnabled(hasDocument && !isConverting);
    ui->actionSave->setEnabled(hasMarkdown && !isConverting);
}
