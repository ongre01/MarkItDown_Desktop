#ifndef DOCUMENTCONTROLLER_H
#define DOCUMENTCONTROLLER_H

#include "../model/ConversionError.h"

#include <QObject>
#include <QString>

#include <memory>

class IMarkItDownManager;

class DocumentController : public QObject
{
    Q_OBJECT

public:
    explicit DocumentController(QObject *parent = nullptr);
    DocumentController(std::unique_ptr<IMarkItDownManager> markItDown,
                       QObject *parent = nullptr);
    ~DocumentController() override;

    void convert(const QString &filePath);
    bool isConverting() const;

signals:
    void conversionStarted();
    void conversionFinished(const QString &markdown);
    void conversionFailed(ConversionError error, const QString &details);

private:
    const std::unique_ptr<IMarkItDownManager> m_markItDown;
};

#endif // DOCUMENTCONTROLLER_H
