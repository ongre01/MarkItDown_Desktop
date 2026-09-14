#include "DocumentFileOperations.h"

#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QSaveFile>

namespace {

const QStringList &supportedExtensions()
{
    static const QStringList extensions{
        QStringLiteral("pdf"),
        QStringLiteral("docx"),
        QStringLiteral("pptx"),
        QStringLiteral("xlsx"),
        QStringLiteral("xls"),
        QStringLiteral("html"),
        QStringLiteral("htm"),
        QStringLiteral("csv"),
        QStringLiteral("json"),
        QStringLiteral("xml"),
        QStringLiteral("txt")};

    return extensions;
}

} // namespace

namespace DocumentFileOperations {

QStringList supportedNameFilters()
{
    QStringList patterns;
    patterns.reserve(supportedExtensions().size());

    for (const QString &extension : supportedExtensions()) {
        patterns.append(QStringLiteral("*.%1").arg(extension));
    }

    return patterns;
}

SourceDocumentValidation validateSourceDocument(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    SourceDocumentValidation result;
    result.absoluteFilePath = fileInfo.absoluteFilePath();
    result.extension = fileInfo.suffix();

    if (fileInfo.isDir()) {
        result.error = SourceDocumentError::Directory;
    } else if (!fileInfo.exists()) {
        result.error = SourceDocumentError::NotFound;
    } else if (!fileInfo.isFile()) {
        result.error = SourceDocumentError::NotFile;
    } else if (!supportedExtensions().contains(result.extension,
                                                Qt::CaseInsensitive)) {
        result.error = SourceDocumentError::UnsupportedExtension;
    }

    return result;
}

QString suggestedMarkdownPath(const QString &sourceFilePath)
{
    const QFileInfo sourceInfo(sourceFilePath);
    return sourceInfo.dir().filePath(
        sourceInfo.completeBaseName() + QStringLiteral(".md"));
}

QString normalizedMarkdownPath(const QString &filePath)
{
    QString normalizedPath = filePath;
    if (QFileInfo(normalizedPath).suffix().isEmpty()) {
        normalizedPath += QStringLiteral(".md");
    }

    return QFileInfo(normalizedPath).absoluteFilePath();
}

MarkdownWriteResult writeMarkdownUtf8(const QString &filePath,
                                      const QString &markdown)
{
    MarkdownWriteResult result;
    result.absoluteFilePath = QFileInfo(filePath).absoluteFilePath();

    QSaveFile outputFile(result.absoluteFilePath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        result.errorMessage = outputFile.errorString();
        return result;
    }

    const QByteArray utf8 = markdown.toUtf8();
    if (outputFile.write(utf8) != utf8.size()) {
        result.errorMessage = outputFile.errorString();
        outputFile.cancelWriting();
        return result;
    }

    if (!outputFile.commit()) {
        result.errorMessage = outputFile.errorString();
        return result;
    }

    result.succeeded = true;
    return result;
}

} // namespace DocumentFileOperations
