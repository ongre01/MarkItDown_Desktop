#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>

enum class DocumentStatus
{
    Empty,
    Ready,
    Converting,
    Completed,
    Failed
};

struct Document
{
    QString sourceFilePath;
    QString markdown;
    QString markdownFilePath;

    bool modified = false;

    DocumentStatus status = DocumentStatus::Empty;
};

#endif // DOCUMENT_H
