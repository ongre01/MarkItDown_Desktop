#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "src/model/Document.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();

private:
    void updateDocumentPresentation();

    Ui::MainWindow *ui;
    Document m_document;
};
#endif // MAINWINDOW_H
