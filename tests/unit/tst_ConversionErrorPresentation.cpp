#include "src/ui/ConversionErrorPresentation.h"

#include <QtTest>

class TestConversionErrorPresentation : public QObject
{
    Q_OBJECT

private slots:
    void presentation_definedError_returnsExpectedText_data();
    void presentation_definedError_returnsExpectedText();
};

void TestConversionErrorPresentation::presentation_definedError_returnsExpectedText_data()
{
    QTest::addColumn<ConversionError>("error");
    QTest::addColumn<QString>("expectedTitle");
    QTest::addColumn<QString>("expectedSummary");

    QTest::newRow("AlreadyRunning")
        << ConversionError::AlreadyRunning
        << QStringLiteral("Conversion Already Running")
        << QString::fromUtf8(u8"다른 문서 변환이 이미 실행 중입니다.");
    QTest::newRow("ExecutableNotFound")
        << ConversionError::ExecutableNotFound
        << QStringLiteral("MarkItDown Not Found")
        << QString::fromUtf8(
               u8"MarkItDown 실행 파일을 찾을 수 없습니다. 앱 로컬 백엔드 설치 또는 "
               u8"MARKITDOWN_EXECUTABLE 설정을 확인하세요.");
    QTest::newRow("FailedToStart")
        << ConversionError::FailedToStart
        << QStringLiteral("MarkItDown Start Failed")
        << QString::fromUtf8(u8"MarkItDown 프로세스를 시작하지 못했습니다.");
    QTest::newRow("Crashed")
        << ConversionError::Crashed
        << QStringLiteral("MarkItDown Crashed")
        << QString::fromUtf8(u8"변환 중 MarkItDown 프로세스가 비정상 종료되었습니다.");
    QTest::newRow("NonZeroExit")
        << ConversionError::NonZeroExit
        << QStringLiteral("Conversion Command Failed")
        << QString::fromUtf8(u8"MarkItDown 변환 명령이 오류 종료 코드를 반환했습니다.");
    QTest::newRow("EmptyOutput")
        << ConversionError::EmptyOutput
        << QStringLiteral("Empty Conversion Output")
        << QString::fromUtf8(u8"MarkItDown 변환 결과가 비어 있습니다.");
    QTest::newRow("ProcessFailure")
        << ConversionError::ProcessFailure
        << QStringLiteral("MarkItDown Execution Failed")
        << QString::fromUtf8(u8"MarkItDown 프로세스 실행 중 오류가 발생했습니다.");
}

void TestConversionErrorPresentation::presentation_definedError_returnsExpectedText()
{
    // Arrange
    QFETCH(ConversionError, error);
    QFETCH(QString, expectedTitle);
    QFETCH(QString, expectedSummary);

    // Act
    const ConversionErrorPresentation presentation =
        conversionErrorPresentation(error);

    // Assert
    QCOMPARE(presentation.title, expectedTitle);
    QCOMPARE(presentation.summary, expectedSummary);
    QVERIFY(!presentation.title.trimmed().isEmpty());
    QVERIFY(!presentation.summary.trimmed().isEmpty());
}

QTEST_APPLESS_MAIN(TestConversionErrorPresentation)

#include "tst_ConversionErrorPresentation.moc"
