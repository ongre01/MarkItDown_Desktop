#include "src/io/DocumentFileOperations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

class TestDocumentFileOperations : public QObject
{
    Q_OBJECT

private slots:
    void validateSourceDocument_supportedExtension_returnsValid_data();
    void validateSourceDocument_supportedExtension_returnsValid();
    void validateSourceDocument_emptyPath_returnsNotFound();
    void validateSourceDocument_relativePath_returnsAbsolutePath();
    void validateSourceDocument_unicodeNameWithSpaces_returnsValid();
    void validateSourceDocument_missingFile_returnsNotFound();
    void validateSourceDocument_directory_returnsDirectoryError();
    void validateSourceDocument_unsupportedExtension_returnsUnsupportedExtension();
    void suggestedMarkdownPath_sourceWithMultipleSuffixes_replacesFinalSuffix();
    void normalizedMarkdownPath_withoutExtension_addsMdExtension();
    void normalizedMarkdownPath_withExtension_preservesExtension();
    void writeMarkdownUtf8_unicodeText_preservesUtf8Content();
    void writeMarkdownUtf8_emptyMarkdown_createsEmptyFile();
    void writeMarkdownUtf8_largeMarkdown_preservesAllContent();
    void writeMarkdownUtf8_existingDestination_replacesContent();
    void writeMarkdownUtf8_missingParentDirectory_returnsFailure();
};

void TestDocumentFileOperations::
    validateSourceDocument_supportedExtension_returnsValid_data()
{
    QTest::addColumn<QString>("extension");

    const QStringList extensions{
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
        QStringLiteral("txt"),
        QStringLiteral("PDF")};

    for (const QString &extension : extensions) {
        QTest::newRow(extension.toUtf8().constData()) << extension;
    }
}

void TestDocumentFileOperations::
    validateSourceDocument_supportedExtension_returnsValid()
{
    // Arrange
    QFETCH(QString, extension);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QTemporaryFile sourceFile(
        QDir(tempDir.path()).filePath(QStringLiteral("source.XXXXXX.%1").arg(extension)));
    QVERIFY(sourceFile.open());

    // Act
    const auto result =
        DocumentFileOperations::validateSourceDocument(sourceFile.fileName());

    // Assert
    QVERIFY(result.isValid());
    QCOMPARE(result.error, DocumentFileOperations::SourceDocumentError::None);
    QCOMPARE(result.absoluteFilePath, QFileInfo(sourceFile.fileName()).absoluteFilePath());
    QCOMPARE(result.extension, extension);
}

void TestDocumentFileOperations::
    validateSourceDocument_emptyPath_returnsNotFound()
{
    // Arrange / Act
    const auto result = DocumentFileOperations::validateSourceDocument(QString());

    // Assert
    QVERIFY(!result.isValid());
    QCOMPARE(result.error, DocumentFileOperations::SourceDocumentError::NotFound);
    QVERIFY(result.extension.isEmpty());
}

void TestDocumentFileOperations::
    validateSourceDocument_relativePath_returnsAbsolutePath()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString absolutePath =
        QDir(tempDir.path()).filePath(QStringLiteral("relative-source.pdf"));
    QFile sourceFile(absolutePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.close();

    const QString originalCurrentPath = QDir::currentPath();
    const auto restoreCurrentPath = qScopeGuard([originalCurrentPath]() {
        QDir::setCurrent(originalCurrentPath);
    });
    QVERIFY(QDir::setCurrent(tempDir.path()));

    // Act
    const auto result = DocumentFileOperations::validateSourceDocument(
        QStringLiteral("relative-source.pdf"));

    // Assert
    QVERIFY(result.isValid());
    QCOMPARE(result.error, DocumentFileOperations::SourceDocumentError::None);
    QCOMPARE(result.absoluteFilePath, QFileInfo(absolutePath).absoluteFilePath());
    QCOMPARE(result.extension, QStringLiteral("pdf"));
}

void TestDocumentFileOperations::
    validateSourceDocument_unicodeNameWithSpaces_returnsValid()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = QDir(tempDir.path()).filePath(
        QString::fromUtf8(u8"자료 보고서.PdF"));
    QFile sourceFile(filePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.close();

    // Act
    const auto result = DocumentFileOperations::validateSourceDocument(filePath);

    // Assert
    QVERIFY(result.isValid());
    QCOMPARE(result.error, DocumentFileOperations::SourceDocumentError::None);
    QCOMPARE(result.absoluteFilePath, QFileInfo(filePath).absoluteFilePath());
    QCOMPARE(result.extension, QStringLiteral("PdF"));
}

void TestDocumentFileOperations::
    validateSourceDocument_missingFile_returnsNotFound()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = QDir(tempDir.path()).filePath(QStringLiteral("missing.docx"));

    // Act
    const auto result = DocumentFileOperations::validateSourceDocument(filePath);

    // Assert
    QVERIFY(!result.isValid());
    QCOMPARE(result.error, DocumentFileOperations::SourceDocumentError::NotFound);
    QCOMPARE(result.absoluteFilePath, QFileInfo(filePath).absoluteFilePath());
    QCOMPARE(result.extension, QStringLiteral("docx"));
}

void TestDocumentFileOperations::
    validateSourceDocument_directory_returnsDirectoryError()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Act
    const auto result = DocumentFileOperations::validateSourceDocument(tempDir.path());

    // Assert
    QVERIFY(!result.isValid());
    QCOMPARE(result.error, DocumentFileOperations::SourceDocumentError::Directory);
    QCOMPARE(result.absoluteFilePath, QFileInfo(tempDir.path()).absoluteFilePath());
}

void TestDocumentFileOperations::
    validateSourceDocument_unsupportedExtension_returnsUnsupportedExtension()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QTemporaryFile sourceFile(
        QDir(tempDir.path()).filePath(QStringLiteral("source.XXXXXX.zip")));
    QVERIFY(sourceFile.open());

    // Act
    const auto result =
        DocumentFileOperations::validateSourceDocument(sourceFile.fileName());

    // Assert
    QVERIFY(!result.isValid());
    QCOMPARE(result.error,
             DocumentFileOperations::SourceDocumentError::UnsupportedExtension);
    QCOMPARE(result.extension, QStringLiteral("zip"));
}

void TestDocumentFileOperations::
    suggestedMarkdownPath_sourceWithMultipleSuffixes_replacesFinalSuffix()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourcePath =
        QDir(tempDir.path()).filePath(QStringLiteral("report.final.pdf"));

    // Act
    const QString result = DocumentFileOperations::suggestedMarkdownPath(sourcePath);

    // Assert
    QCOMPARE(result,
             QDir(tempDir.path()).filePath(QStringLiteral("report.final.md")));
}

void TestDocumentFileOperations::
    normalizedMarkdownPath_withoutExtension_addsMdExtension()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString requestedPath =
        QDir(tempDir.path()).filePath(QStringLiteral("converted-document"));

    // Act
    const QString result =
        DocumentFileOperations::normalizedMarkdownPath(requestedPath);

    // Assert
    QCOMPARE(result,
             QFileInfo(requestedPath + QStringLiteral(".md")).absoluteFilePath());
}

void TestDocumentFileOperations::
    normalizedMarkdownPath_withExtension_preservesExtension()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString requestedPath =
        QDir(tempDir.path()).filePath(QStringLiteral("converted-document.MD"));

    // Act
    const QString result =
        DocumentFileOperations::normalizedMarkdownPath(requestedPath);

    // Assert
    QCOMPARE(result, QFileInfo(requestedPath).absoluteFilePath());
}

void TestDocumentFileOperations::writeMarkdownUtf8_unicodeText_preservesUtf8Content()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = QDir(tempDir.path()).filePath(QStringLiteral("unicode.md"));
    const QString markdown = QString::fromUtf8(
        u8"# English\n\n한글 문장\n\n日本語の文章\n");

    // Act
    const auto result = DocumentFileOperations::writeMarkdownUtf8(filePath, markdown);

    // Assert
    QVERIFY2(result.succeeded, qPrintable(result.errorMessage));
    QCOMPARE(result.absoluteFilePath, QFileInfo(filePath).absoluteFilePath());
    QVERIFY(result.errorMessage.isEmpty());

    QFile savedFile(result.absoluteFilePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    const QByteArray bytes = savedFile.readAll();
    QCOMPARE(bytes, markdown.toUtf8());
    QVERIFY(!bytes.startsWith(QByteArray::fromHex("efbbbf")));
    QCOMPARE(QString::fromUtf8(bytes), markdown);
}

void TestDocumentFileOperations::
    writeMarkdownUtf8_emptyMarkdown_createsEmptyFile()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath =
        QDir(tempDir.path()).filePath(QStringLiteral("empty.md"));

    // Act
    const auto result = DocumentFileOperations::writeMarkdownUtf8(filePath, {});

    // Assert
    QVERIFY2(result.succeeded, qPrintable(result.errorMessage));
    QFile savedFile(result.absoluteFilePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    QCOMPARE(savedFile.readAll(), QByteArray());
}

void TestDocumentFileOperations::
    writeMarkdownUtf8_largeMarkdown_preservesAllContent()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath =
        QDir(tempDir.path()).filePath(QStringLiteral("large.md"));
    const QString markdown(1024 * 1024, QLatin1Char('x'));

    // Act
    const auto result = DocumentFileOperations::writeMarkdownUtf8(filePath, markdown);

    // Assert
    QVERIFY2(result.succeeded, qPrintable(result.errorMessage));
    QFile savedFile(result.absoluteFilePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    QCOMPARE(savedFile.readAll(), markdown.toUtf8());
}

void TestDocumentFileOperations::
    writeMarkdownUtf8_existingDestination_replacesContent()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath =
        QDir(tempDir.path()).filePath(QStringLiteral("existing.md"));
    QFile existingFile(filePath);
    QVERIFY(existingFile.open(QIODevice::WriteOnly));
    QCOMPARE(existingFile.write(QByteArrayLiteral("old content")),
             qsizetype(11));
    existingFile.close();
    const QString replacement = QString::fromUtf8(u8"# 새 내용\n");

    // Act
    const auto result =
        DocumentFileOperations::writeMarkdownUtf8(filePath, replacement);

    // Assert
    QVERIFY2(result.succeeded, qPrintable(result.errorMessage));
    QCOMPARE(result.absoluteFilePath, QFileInfo(filePath).absoluteFilePath());
    QFile savedFile(filePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    QCOMPARE(savedFile.readAll(), replacement.toUtf8());
}

void TestDocumentFileOperations::
    writeMarkdownUtf8_missingParentDirectory_returnsFailure()
{
    // Arrange
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = QDir(tempDir.path()).filePath(
        QStringLiteral("missing-parent/output.md"));

    // Act
    const auto result = DocumentFileOperations::writeMarkdownUtf8(
        filePath, QStringLiteral("content"));

    // Assert
    QVERIFY(!result.succeeded);
    QCOMPARE(result.absoluteFilePath, QFileInfo(filePath).absoluteFilePath());
    QVERIFY(!result.errorMessage.trimmed().isEmpty());
    QVERIFY(!QFileInfo::exists(filePath));
}

QTEST_APPLESS_MAIN(TestDocumentFileOperations)

#include "tst_DocumentFileOperations.moc"
