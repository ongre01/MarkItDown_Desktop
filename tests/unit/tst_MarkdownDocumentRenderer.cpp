#include "src/rendering/MarkdownDocumentRenderer.h"

#include <QFont>
#include <QSignalSpy>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>
#include <QtTest>

class TestMarkdownDocumentRenderer : public QObject
{
    Q_OBJECT

private slots:
    void render_empty_emitsEmptyDocument();
    void render_heading_containsHeadingTextAndLevel();
    void render_bold_preservesBoldFormatting();
    void render_multiline_preservesLineOrder();
    void render_unicode_preservesCharacters();

private:
    static QString renderAndCaptureHtml(const QString &markdown, quint64 requestId);
};

QString TestMarkdownDocumentRenderer::renderAndCaptureHtml(const QString &markdown,
                                                            quint64 requestId)
{
    MarkdownDocumentRenderer renderer;
    QSignalSpy renderedSpy(&renderer, &MarkdownDocumentRenderer::rendered);

    renderer.render(requestId, markdown);

    if (renderedSpy.count() != 1) {
        return QString();
    }

    const QList<QVariant> arguments = renderedSpy.takeFirst();
    if (arguments.size() != 2 || arguments.at(0).toULongLong() != requestId) {
        return QString();
    }

    return arguments.at(1).toString();
}

void TestMarkdownDocumentRenderer::render_empty_emitsEmptyDocument()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(QString(), 101);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QVERIFY(!html.isEmpty());
    QVERIFY(document.toPlainText().isEmpty());
}

void TestMarkdownDocumentRenderer::render_heading_containsHeadingTextAndLevel()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(QStringLiteral("# Protocol Title"), 102);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QCOMPARE(document.toPlainText(), QStringLiteral("Protocol Title"));
    QCOMPARE(document.begin().blockFormat().headingLevel(), 1);
}

void TestMarkdownDocumentRenderer::render_bold_preservesBoldFormatting()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(
        QStringLiteral("Normal **important** text"), 103);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QCOMPARE(document.toPlainText(), QStringLiteral("Normal important text"));

    bool foundBoldText = false;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        for (auto fragmentIterator = block.begin(); !fragmentIterator.atEnd();
             ++fragmentIterator) {
            const QTextFragment fragment = fragmentIterator.fragment();
            if (fragment.isValid()
                && fragment.text().contains(QStringLiteral("important"))
                && fragment.charFormat().fontWeight() >= QFont::Bold) {
                foundBoldText = true;
            }
        }
    }

    QVERIFY(foundBoldText);
}

void TestMarkdownDocumentRenderer::render_multiline_preservesLineOrder()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(
        QStringLiteral("First paragraph.\n\nSecond paragraph."), 104);
    QTextDocument document;
    document.setHtml(html);
    const QString plainText = document.toPlainText();

    // Assert
    const qsizetype firstIndex = plainText.indexOf(QStringLiteral("First paragraph."));
    const qsizetype secondIndex = plainText.indexOf(QStringLiteral("Second paragraph."));
    QVERIFY(firstIndex >= 0);
    QVERIFY(secondIndex > firstIndex);
}

void TestMarkdownDocumentRenderer::render_unicode_preservesCharacters()
{
    // Arrange
    const QString markdown = QString::fromUtf8(
        u8"# Unicode\n\nEnglish 한글 日本語");

    // Act
    const QString html = renderAndCaptureHtml(markdown, 105);
    QTextDocument document;
    document.setHtml(html);
    const QString plainText = document.toPlainText();

    // Assert
    QVERIFY(plainText.contains(QStringLiteral("English")));
    QVERIFY(plainText.contains(QString::fromUtf8(u8"한글")));
    QVERIFY(plainText.contains(QString::fromUtf8(u8"日本語")));
}

QTEST_GUILESS_MAIN(TestMarkdownDocumentRenderer)

#include "tst_MarkdownDocumentRenderer.moc"
