#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "src/model/Document.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class DocumentController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void convertFile();
    void onConversionStarted();
    void onConversionFinished(const QString &markdown);
    void onConversionFailed(const QString &error);

private:
    void updateDocumentPresentation();

    Ui::MainWindow *ui;
    DocumentController *m_controller;
    Document m_document;
};
#endif // MAINWINDOW_H
