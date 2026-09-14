#ifndef DOCUMENTCONTROLLER_H
#define DOCUMENTCONTROLLER_H

#include "../model/ConversionError.h"

#include <QObject>
#include <QString>

class MarkItDownManager;

class DocumentController : public QObject
{
    Q_OBJECT

public:
    explicit DocumentController(QObject *parent = nullptr);

    void convert(const QString &filePath);
    bool isConverting() const;

signals:
    void conversionStarted();
    void conversionFinished(const QString &markdown);
    void conversionFailed(ConversionError error, const QString &details);

private:
    MarkItDownManager *const m_markItDown;
};

#endif // DOCUMENTCONTROLLER_H
