#include "src/rendering/MarkdownDocumentRenderer.h"

#include <QFont>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextList>
#include <QTextListFormat>
#include <QtTest>

class TestMarkdownDocumentRenderer : public QObject
{
    Q_OBJECT

private slots:
    void render_empty_emitsEmptyDocument();
    void render_heading_containsHeadingTextAndLevel_data();
    void render_heading_containsHeadingTextAndLevel();
    void render_italic_preservesItalicFormatting();
    void render_bold_preservesBoldFormatting();
    void render_list_preservesItemsAndListStyle_data();
    void render_list_preservesItemsAndListStyle();
    void render_link_preservesAnchorTarget();
    void render_codeBlock_preservesTextAndFixedPitchFormatting();
    void render_blockquote_preservesTextAndQuoteIndentation();
    void render_multiline_preservesLineOrder();
    void render_unicode_preservesCharacters();
    void render_specialCharacters_preservesLiteralText();
    void render_largeDocument_preservesFirstAndLastSections();

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

void TestMarkdownDocumentRenderer::
    render_heading_containsHeadingTextAndLevel_data()
{
    QTest::addColumn<int>("level");

    for (int level = 1; level <= 6; ++level) {
        QTest::newRow(qPrintable(QStringLiteral("level-%1").arg(level))) << level;
    }
}

void TestMarkdownDocumentRenderer::render_heading_containsHeadingTextAndLevel()
{
    // Arrange
    QFETCH(int, level);
    const QString headingText = QStringLiteral("Heading Level %1").arg(level);
    const QString markdown = QString(level, QLatin1Char('#'))
                             + QLatin1Char(' ') + headingText;

    // Act
    const QString html = renderAndCaptureHtml(markdown, 102);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QCOMPARE(document.toPlainText(), headingText);
    QCOMPARE(document.begin().blockFormat().headingLevel(), level);
}

void TestMarkdownDocumentRenderer::render_italic_preservesItalicFormatting()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(
        QStringLiteral("Normal *emphasized* text"), 103);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QCOMPARE(document.toPlainText(), QStringLiteral("Normal emphasized text"));

    bool foundItalicText = false;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        for (auto fragmentIterator = block.begin(); !fragmentIterator.atEnd();
             ++fragmentIterator) {
            const QTextFragment fragment = fragmentIterator.fragment();
            if (fragment.isValid()
                && fragment.text().contains(QStringLiteral("emphasized"))
                && fragment.charFormat().fontItalic()) {
                foundItalicText = true;
            }
        }
    }

    QVERIFY(foundItalicText);
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

void TestMarkdownDocumentRenderer::render_list_preservesItemsAndListStyle_data()
{
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<int>("expectedStyle");

    QTest::newRow("unordered")
        << QStringLiteral("- alpha\n- beta")
        << static_cast<int>(QTextListFormat::ListDisc);
    QTest::newRow("ordered")
        << QStringLiteral("1. alpha\n2. beta")
        << static_cast<int>(QTextListFormat::ListDecimal);
}

void TestMarkdownDocumentRenderer::render_list_preservesItemsAndListStyle()
{
    // Arrange
    QFETCH(QString, markdown);
    QFETCH(int, expectedStyle);

    // Act
    const QString html = renderAndCaptureHtml(markdown, 104);
    QTextDocument document;
    document.setHtml(html);
    QTextBlock firstItem = document.begin();
    QTextList *list = firstItem.textList();

    // Assert
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 2);
    QCOMPARE(list->item(0).text(), QStringLiteral("alpha"));
    QCOMPARE(list->item(1).text(), QStringLiteral("beta"));
    QCOMPARE(static_cast<int>(list->format().style()), expectedStyle);
}

void TestMarkdownDocumentRenderer::render_link_preservesAnchorTarget()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(
        QStringLiteral("Read the [reference](https://example.com/reference)."),
        105);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QCOMPARE(document.toPlainText(), QStringLiteral("Read the reference."));

    bool foundLink = false;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        for (auto fragmentIterator = block.begin(); !fragmentIterator.atEnd();
             ++fragmentIterator) {
            const QTextFragment fragment = fragmentIterator.fragment();
            if (fragment.isValid()
                && fragment.text() == QStringLiteral("reference")
                && fragment.charFormat().isAnchor()
                && fragment.charFormat().anchorHref()
                       == QStringLiteral("https://example.com/reference")) {
                foundLink = true;
            }
        }
    }

    QVERIFY(foundLink);
}

void TestMarkdownDocumentRenderer::
    render_codeBlock_preservesTextAndFixedPitchFormatting()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(
        QStringLiteral("```cpp\nint value = 42;\n```"), 106);

    // Assert. Re-loading Qt's fenced-code HTML into a second QTextDocument triggers
    // a Qt 6.11 debug assertion, so verify the semantic preformatted element without
    // comparing the complete serialized document.
    QVERIFY(html.contains(QStringLiteral("<pre")));
    QVERIFY(html.contains(QStringLiteral("int value = 42;")));
}

void TestMarkdownDocumentRenderer::
    render_blockquote_preservesTextAndQuoteIndentation()
{
    // Arrange / Act
    const QString html = renderAndCaptureHtml(
        QStringLiteral("> quoted protocol note"), 107);
    QTextDocument document;
    document.setHtml(html);
    const QTextBlock quoteBlock = document.begin();

    // Assert
    QCOMPARE(quoteBlock.text(), QStringLiteral("quoted protocol note"));
    const QRegularExpression nonZeroLeftMargin(
        QStringLiteral("margin-left:\\s*(?:[1-9]\\d*(?:\\.\\d+)?|"
                       "0\\.\\d*[1-9]\\d*)px"));
    QVERIFY(nonZeroLeftMargin.match(html).hasMatch());
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

void TestMarkdownDocumentRenderer::
    render_specialCharacters_preservesLiteralText()
{
    // Arrange
    const QString specialText = QString::fromUtf8(
        u8"Symbols: & < > \" ' © ™ →");

    // Act
    const QString html = renderAndCaptureHtml(specialText, 108);
    QTextDocument document;
    document.setHtml(html);

    // Assert
    QCOMPARE(document.toPlainText(), specialText);
}

void TestMarkdownDocumentRenderer::
    render_largeDocument_preservesFirstAndLastSections()
{
    // Arrange
    constexpr int SectionCount = 2000;
    QString markdown;
    markdown.reserve(SectionCount * 48);
    for (int section = 1; section <= SectionCount; ++section) {
        markdown += QStringLiteral("## Section %1\n\nPayload %1 한글\n\n")
                        .arg(section);
    }

    // Act
    const QString html = renderAndCaptureHtml(markdown, 109);
    QTextDocument document;
    document.setHtml(html);
    const QString plainText = document.toPlainText();

    // Assert
    QVERIFY(plainText.startsWith(QStringLiteral("Section 1")));
    QVERIFY(plainText.contains(QString::fromUtf8(u8"Payload 1 한글")));
    QVERIFY(plainText.contains(QStringLiteral("Section 2000")));
    QVERIFY(plainText.contains(QString::fromUtf8(u8"Payload 2000 한글")));
    QVERIFY(document.blockCount() >= SectionCount * 2);
}

QTEST_GUILESS_MAIN(TestMarkdownDocumentRenderer)

#include "tst_MarkdownDocumentRenderer.moc"
