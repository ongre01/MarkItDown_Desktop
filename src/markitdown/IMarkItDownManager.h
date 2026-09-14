#ifndef IMARKITDOWNMANAGER_H
#define IMARKITDOWNMANAGER_H

#include "../model/ConversionError.h"

#include <QObject>
#include <QString>

class IMarkItDownManager : public QObject
{
    Q_OBJECT

public:
    explicit IMarkItDownManager(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~IMarkItDownManager() override = default;

    virtual void convert(const QString &filePath) = 0;
    virtual bool isRunning() const = 0;

signals:
    void started();
    void finished(const QString &markdown);
    void failed(ConversionError error, const QString &details);
};

#endif // IMARKITDOWNMANAGER_H
