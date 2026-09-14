#ifndef DOCUMENTFILEOPERATIONS_H
#define DOCUMENTFILEOPERATIONS_H

#include <QString>
#include <QStringList>

namespace DocumentFileOperations {

enum class SourceDocumentError
{
    None,
    Directory,
    NotFound,
    NotFile,
    UnsupportedExtension
};

struct SourceDocumentValidation
{
    SourceDocumentError error = SourceDocumentError::None;
    QString absoluteFilePath;
    QString extension;

    bool isValid() const noexcept
    {
        return error == SourceDocumentError::None;
    }
};

struct MarkdownWriteResult
{
    bool succeeded = false;
    QString absoluteFilePath;
    QString errorMessage;
};

QStringList supportedNameFilters();
SourceDocumentValidation validateSourceDocument(const QString &filePath);
QString suggestedMarkdownPath(const QString &sourceFilePath);
QString normalizedMarkdownPath(const QString &filePath);
MarkdownWriteResult writeMarkdownUtf8(const QString &filePath,
                                      const QString &markdown);

} // namespace DocumentFileOperations

#endif // DOCUMENTFILEOPERATIONS_H
