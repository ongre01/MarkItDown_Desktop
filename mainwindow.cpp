#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->showMessage(tr("Ready"));

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
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

    updateDocumentPresentation();
}

void MainWindow::updateDocumentPresentation()
{
    const QString fileName = QFileInfo(m_document.sourceFilePath).fileName();

    setWindowTitle(tr("MarkItDown Viewer - %1").arg(fileName));
    ui->statusbar->showMessage(tr("%1 | Ready").arg(fileName));
}
