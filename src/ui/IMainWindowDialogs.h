#ifndef IMAINWINDOWDIALOGS_H
#define IMAINWINDOWDIALOGS_H

#include <QString>

class QWidget;

class IMainWindowDialogs
{
public:
    virtual ~IMainWindowDialogs() = default;

    virtual QString selectSourceDocument(QWidget *parent,
                                         const QString &filter) = 0;
    virtual QString selectMarkdownDestination(QWidget *parent,
                                              const QString &suggestedFilePath) = 0;
    virtual void showWarning(QWidget *parent,
                             const QString &title,
                             const QString &message) = 0;
    virtual void showCritical(QWidget *parent,
                              const QString &title,
                              const QString &message) = 0;
};

#endif // IMAINWINDOWDIALOGS_H
