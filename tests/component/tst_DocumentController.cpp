#include "src/controller/DocumentController.h"
#include "src/markitdown/IMarkItDownManager.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

namespace {

class FakeMarkItDownManager final : public IMarkItDownManager
{
public:
    void convert(const QString &filePath) override
    {
        ++m_convertCallCount;
        m_sourcePath = filePath;
    }

    bool isRunning() const override
    {
        return m_running;
    }

    void setRunning(bool running)
    {
        m_running = running;
    }

    void reportStarted()
    {
        emit started();
    }

    void reportFinished(const QString &markdown)
    {
        emit finished(markdown);
    }

    void reportFailed(ConversionError error, const QString &details)
    {
        emit failed(error, details);
    }

    int convertCallCount() const
    {
        return m_convertCallCount;
    }

    QString sourcePath() const
    {
        return m_sourcePath;
    }

private:
    bool m_running = false;
    int m_convertCallCount = 0;
    QString m_sourcePath;
};

std::unique_ptr<IMarkItDownManager> createManager(
    FakeMarkItDownManager *&fakeManager)
{
    auto manager = std::make_unique<FakeMarkItDownManager>();
    fakeManager = manager.get();
    return manager;
}

class ControllerHarness
{
public:
    ControllerHarness()
        : controller(createManager(manager))
    {
    }

    FakeMarkItDownManager *manager = nullptr;
    DocumentController controller;
};

} // namespace

class TestDocumentController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void convert_sourcePath_delegatesExactlyOnce();
    void isConverting_managerState_matchesManagerState();
    void managerStarted_emitted_forwardsConversionStarted();
    void managerFinished_emitted_forwardsMarkdown();
    void managerFailed_emitted_forwardsErrorAndDetails_data();
    void managerFailed_emitted_forwardsErrorAndDetails();
};

void TestDocumentController::initTestCase()
{
    qRegisterMetaType<ConversionError>();
}

void TestDocumentController::convert_sourcePath_delegatesExactlyOnce()
{
    // Arrange
    ControllerHarness harness;
    const QString sourcePath =
        QString::fromUtf8(u8"C:/문서/Input Document.docx");

    // Act
    harness.controller.convert(sourcePath);

    // Assert
    QCOMPARE(harness.manager->convertCallCount(), 1);
    QCOMPARE(harness.manager->sourcePath(), sourcePath);
}

void TestDocumentController::isConverting_managerState_matchesManagerState()
{
    // Arrange
    ControllerHarness harness;

    // Act / Assert: the Controller does not keep a duplicate running state.
    harness.manager->setRunning(false);
    QVERIFY(!harness.controller.isConverting());

    harness.manager->setRunning(true);
    QVERIFY(harness.controller.isConverting());
}

void TestDocumentController::
    managerStarted_emitted_forwardsConversionStarted()
{
    // Arrange
    ControllerHarness harness;
    QSignalSpy startedSpy(&harness.controller,
                          &DocumentController::conversionStarted);

    // Act
    harness.manager->reportStarted();

    // Assert
    QCOMPARE(startedSpy.count(), 1);
}

void TestDocumentController::managerFinished_emitted_forwardsMarkdown()
{
    // Arrange
    ControllerHarness harness;
    QSignalSpy finishedSpy(&harness.controller,
                           &DocumentController::conversionFinished);
    QSignalSpy failedSpy(&harness.controller,
                         &DocumentController::conversionFailed);
    const QString markdown =
        QString::fromUtf8(u8"# Markdown Result\n\n변환 본문");

    // Act
    harness.manager->reportFinished(markdown);

    // Assert
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.at(0).at(0).toString(), markdown);
    QCOMPARE(failedSpy.count(), 0);
}

void TestDocumentController::
    managerFailed_emitted_forwardsErrorAndDetails_data()
{
    QTest::addColumn<ConversionError>("error");

    QTest::newRow("already-running") << ConversionError::AlreadyRunning;
    QTest::newRow("executable-not-found")
        << ConversionError::ExecutableNotFound;
    QTest::newRow("failed-to-start") << ConversionError::FailedToStart;
    QTest::newRow("crashed") << ConversionError::Crashed;
    QTest::newRow("non-zero-exit") << ConversionError::NonZeroExit;
    QTest::newRow("empty-output") << ConversionError::EmptyOutput;
    QTest::newRow("process-failure") << ConversionError::ProcessFailure;
}

void TestDocumentController::
    managerFailed_emitted_forwardsErrorAndDetails()
{
    // Arrange
    QFETCH(ConversionError, error);
    ControllerHarness harness;
    QSignalSpy finishedSpy(&harness.controller,
                           &DocumentController::conversionFinished);
    QSignalSpy failedSpy(&harness.controller,
                         &DocumentController::conversionFailed);
    const QString details =
        QString::fromUtf8(u8"backend detail\n표준 오류");

    // Act
    harness.manager->reportFailed(error, details);

    // Assert
    QCOMPARE(failedSpy.count(), 1);
    const QList<QVariant> arguments = failedSpy.at(0);
    QCOMPARE(arguments.at(0).value<ConversionError>(), error);
    QCOMPARE(arguments.at(1).toString(), details);
    QCOMPARE(finishedSpy.count(), 0);
}

QTEST_APPLESS_MAIN(TestDocumentController)

#include "tst_DocumentController.moc"
