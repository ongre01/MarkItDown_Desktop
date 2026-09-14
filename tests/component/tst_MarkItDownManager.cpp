#include "src/markitdown/MarkItDownExecutableResolver.h"
#include "src/markitdown/MarkItDownManager.h"

#include <QDir>
#include <QSignalSpy>
#include <QtTest>

#include <memory>
#include <utility>

namespace {

class FakeProcessRunner final : public IProcessRunner
{
public:
    bool isRunning() const override
    {
        return m_running;
    }

    void setProcessEnvironment(const QProcessEnvironment &environment) override
    {
        m_processEnvironment = environment;
    }

    void start(const QString &program, const QStringList &arguments) override
    {
        ++m_startCallCount;
        m_program = program;
        m_arguments = arguments;
        m_running = true;
        emit started();
    }

    QByteArray readAllStandardOutput() override
    {
        const QByteArray output = m_standardOutput;
        m_standardOutput.clear();
        return output;
    }

    QByteArray readAllStandardError() override
    {
        const QByteArray output = m_standardError;
        m_standardError.clear();
        return output;
    }

    QString errorString() const override
    {
        return m_errorString;
    }

    void setStandardOutput(const QByteArray &output)
    {
        m_standardOutput = output;
    }

    void appendStandardError(const QByteArray &output)
    {
        m_standardError.append(output);
        emit readyReadStandardError();
    }

    void setPendingStandardError(const QByteArray &output)
    {
        m_standardError = output;
    }

    void fail(ProcessError error, const QString &errorString)
    {
        m_errorString = errorString;
        m_running = false;
        emit errorOccurred(error);
    }

    void complete(int exitCode, ExitStatus exitStatus)
    {
        m_running = false;
        emit finished(exitCode, exitStatus);
    }

    int startCallCount() const
    {
        return m_startCallCount;
    }

    QString program() const
    {
        return m_program;
    }

    QStringList arguments() const
    {
        return m_arguments;
    }

    QProcessEnvironment processEnvironment() const
    {
        return m_processEnvironment;
    }

private:
    bool m_running = false;
    int m_startCallCount = 0;
    QString m_program;
    QStringList m_arguments;
    QProcessEnvironment m_processEnvironment;
    QByteArray m_standardOutput;
    QByteArray m_standardError;
    QString m_errorString;
};

class FakeExecutableResolver final : public IMarkItDownExecutableResolver
{
public:
    FakeExecutableResolver(QString executable, QString configuredExecutable)
        : m_executable(std::move(executable))
        , m_configuredExecutable(std::move(configuredExecutable))
    {
    }

    QString resolve() const override
    {
        return m_executable;
    }

    QString configuredExecutable() const override
    {
        return m_configuredExecutable;
    }

private:
    const QString m_executable;
    const QString m_configuredExecutable;
};

std::unique_ptr<IProcessRunner> createProcessRunner(FakeProcessRunner *&fakeRunner)
{
    auto runner = std::make_unique<FakeProcessRunner>();
    fakeRunner = runner.get();
    return runner;
}

class ManagerHarness
{
public:
    explicit ManagerHarness(
        const QString &executable = QStringLiteral("fake-markitdown"),
        const QString &configuredExecutable = {})
        : manager(createProcessRunner(processRunner),
                  std::make_unique<FakeExecutableResolver>(executable,
                                                           configuredExecutable))
    {
    }

    FakeProcessRunner *processRunner = nullptr;
    MarkItDownManager manager;
};

void compareFailure(const QSignalSpy &spy,
                    ConversionError expectedError,
                    const QString &expectedDetails)
{
    QCOMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.at(0);
    QCOMPARE(arguments.at(0).value<ConversionError>(), expectedError);
    QCOMPARE(arguments.at(1).toString(), expectedDetails);
}

} // namespace

class TestMarkItDownManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void convert_success_emitsMarkdownAndCleansRunningState();
    void convert_whileRunning_emitsAlreadyRunning();
    void convert_missingExecutable_emitsExecutableNotFound_data();
    void convert_missingExecutable_emitsExecutableNotFound();
    void process_failedToStart_emitsFailedToStartAndCleansRunningState();
    void process_crashExit_emitsCrashedAndCleansRunningState();
    void process_nonZeroExit_includesExitCodeAndStandardError();
    void process_emptyOutput_emitsEmptyOutputAndCleansRunningState();
    void processError_generalFailure_emitsProcessFailure_data();
    void processError_generalFailure_emitsProcessFailure();
    void processError_followedByFinished_emitsFailureOnce();
    void convert_afterFailure_startsCleanLifecycle();
};

void TestMarkItDownManager::initTestCase()
{
    qRegisterMetaType<ConversionError>();
}

void TestMarkItDownManager::
    convert_success_emitsMarkdownAndCleansRunningState()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy startedSpy(&harness.manager, &MarkItDownManager::started);
    QSignalSpy finishedSpy(&harness.manager, &MarkItDownManager::finished);
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    const QString sourcePath = QStringLiteral("input document.pdf");
    const QString markdown = QString::fromUtf8(u8"# 변환 결과\n\n본문");
    harness.processRunner->setStandardOutput(markdown.toUtf8());

    // Act: starting remains observable until the Fake completes.
    harness.manager.convert(sourcePath);

    // Assert
    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(harness.manager.isRunning());
    QCOMPARE(harness.processRunner->program(), QStringLiteral("fake-markitdown"));
    QCOMPARE(harness.processRunner->arguments(), QStringList{sourcePath});
    QCOMPARE(harness.processRunner->processEnvironment().value(
                 QStringLiteral("PYTHONUTF8")),
             QStringLiteral("1"));
    QCOMPARE(harness.processRunner->processEnvironment().value(
                 QStringLiteral("PYTHONIOENCODING")),
             QStringLiteral("utf-8"));

    harness.processRunner->complete(0, IProcessRunner::ExitStatus::NormalExit);

    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.at(0).at(0).toString(), markdown);
    QCOMPARE(failedSpy.count(), 0);
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::convert_whileRunning_emitsAlreadyRunning()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy startedSpy(&harness.manager, &MarkItDownManager::started);
    QSignalSpy finishedSpy(&harness.manager, &MarkItDownManager::finished);
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.processRunner->setStandardOutput(QByteArrayLiteral("# first result"));
    harness.manager.convert(QStringLiteral("first.pdf"));
    QVERIFY(harness.manager.isRunning());

    // Act
    harness.manager.convert(QStringLiteral("second.docx"));

    // Assert
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(harness.processRunner->startCallCount(), 1);
    QCOMPARE(harness.processRunner->arguments(),
             QStringList{QStringLiteral("first.pdf")});
    QCOMPARE(finishedSpy.count(), 0);
    compareFailure(failedSpy, ConversionError::AlreadyRunning, {});
    QVERIFY(harness.manager.isRunning());

    harness.processRunner->complete(0, IProcessRunner::ExitStatus::NormalExit);
    QVERIFY(!harness.manager.isRunning());
    QCOMPARE(finishedSpy.count(), 1);
}

void TestMarkItDownManager::convert_missingExecutable_emitsExecutableNotFound_data()
{
    QTest::addColumn<QString>("configuredExecutable");

    QTest::newRow("not-configured") << QString();
    QTest::newRow("configured-path-is-invalid")
        << QStringLiteral("C:/missing/markitdown.exe");
}

void TestMarkItDownManager::convert_missingExecutable_emitsExecutableNotFound()
{
    // Arrange
    QFETCH(QString, configuredExecutable);
    ManagerHarness harness({}, configuredExecutable);
    QSignalSpy startedSpy(&harness.manager, &MarkItDownManager::started);
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);

    // Act
    harness.manager.convert(QStringLiteral("input.pdf"));

    // Assert
    const QString expectedDetails =
        configuredExecutable.isEmpty()
            ? QString()
            : QStringLiteral("MARKITDOWN_EXECUTABLE: %1")
                  .arg(QDir::toNativeSeparators(configuredExecutable));
    QCOMPARE(startedSpy.count(), 0);
    QCOMPARE(harness.processRunner->startCallCount(), 0);
    compareFailure(failedSpy,
                   ConversionError::ExecutableNotFound,
                   expectedDetails);
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::
    process_failedToStart_emitsFailedToStartAndCleansRunningState()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("input.pdf"));
    QVERIFY(harness.manager.isRunning());
    harness.processRunner->setPendingStandardError(
        QByteArrayLiteral("not found"));

    // Act
    harness.processRunner->fail(IProcessRunner::ProcessError::FailedToStart,
                                QStringLiteral("Create process failed"));

    // Assert
    compareFailure(failedSpy,
                   ConversionError::FailedToStart,
                   QStringLiteral("Create process failed\n\n"
                                  "Standard error:\nnot found"));
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::process_crashExit_emitsCrashedAndCleansRunningState()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy finishedSpy(&harness.manager, &MarkItDownManager::finished);
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("input.pdf"));
    QVERIFY(harness.manager.isRunning());
    harness.processRunner->appendStandardError(QByteArrayLiteral("fatal crash\n"));

    // Act
    harness.processRunner->complete(-1, IProcessRunner::ExitStatus::CrashExit);

    // Assert
    QCOMPARE(finishedSpy.count(), 0);
    compareFailure(failedSpy,
                   ConversionError::Crashed,
                   QStringLiteral("Standard error:\nfatal crash"));
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::process_nonZeroExit_includesExitCodeAndStandardError()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("input.pdf"));
    const QString standardError = QString::fromUtf8(
        u8"첫 번째 오류\n두 번째 오류");
    const QByteArray encodedStandardError = standardError.toUtf8();
    harness.processRunner->appendStandardError(encodedStandardError.left(5));
    harness.processRunner->appendStandardError(encodedStandardError.mid(5));

    // Act
    harness.processRunner->complete(7, IProcessRunner::ExitStatus::NormalExit);

    // Assert
    compareFailure(failedSpy,
                   ConversionError::NonZeroExit,
                   QStringLiteral("Exit code: 7\n\nStandard error:\n%1")
                       .arg(standardError));
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::
    process_emptyOutput_emitsEmptyOutputAndCleansRunningState()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy finishedSpy(&harness.manager, &MarkItDownManager::finished);
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("input.pdf"));
    QVERIFY(harness.manager.isRunning());

    // Act
    harness.processRunner->complete(0, IProcessRunner::ExitStatus::NormalExit);

    // Assert
    QCOMPARE(finishedSpy.count(), 0);
    compareFailure(failedSpy, ConversionError::EmptyOutput, {});
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::processError_generalFailure_emitsProcessFailure_data()
{
    QTest::addColumn<IProcessRunner::ProcessError>("processError");

    QTest::newRow("timed-out") << IProcessRunner::ProcessError::Timedout;
    QTest::newRow("write-error") << IProcessRunner::ProcessError::WriteError;
    QTest::newRow("read-error") << IProcessRunner::ProcessError::ReadError;
    QTest::newRow("unknown-error") << IProcessRunner::ProcessError::UnknownError;
}

void TestMarkItDownManager::processError_generalFailure_emitsProcessFailure()
{
    // Arrange
    QFETCH(IProcessRunner::ProcessError, processError);
    ManagerHarness harness;
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("input.pdf"));
    QVERIFY(harness.manager.isRunning());
    harness.processRunner->appendStandardError(QByteArrayLiteral("backend stderr"));

    // Act
    harness.processRunner->fail(processError, QStringLiteral("process detail"));

    // Assert
    compareFailure(failedSpy,
                   ConversionError::ProcessFailure,
                   QStringLiteral("process detail\n\n"
                                  "Standard error:\nbackend stderr"));
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::processError_followedByFinished_emitsFailureOnce()
{
    // Arrange
    ManagerHarness harness;
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("input.pdf"));

    // Act: QProcess may report both errorOccurred and finished for one failure.
    harness.processRunner->fail(IProcessRunner::ProcessError::Crashed,
                                QStringLiteral("crash signal"));
    harness.processRunner->complete(-1, IProcessRunner::ExitStatus::CrashExit);

    // Assert
    compareFailure(failedSpy,
                   ConversionError::Crashed,
                   QStringLiteral("crash signal"));
    QVERIFY(!harness.manager.isRunning());
}

void TestMarkItDownManager::convert_afterFailure_startsCleanLifecycle()
{
    // Arrange: finish the first lifecycle with diagnostic stderr.
    ManagerHarness harness;
    QSignalSpy failedSpy(&harness.manager, &MarkItDownManager::failed);
    harness.manager.convert(QStringLiteral("first.pdf"));
    harness.processRunner->appendStandardError(QByteArrayLiteral("stale stderr"));
    harness.processRunner->fail(IProcessRunner::ProcessError::UnknownError,
                                QStringLiteral("first failure"));
    compareFailure(failedSpy,
                   ConversionError::ProcessFailure,
                   QStringLiteral("first failure\n\n"
                                  "Standard error:\nstale stderr"));
    QVERIFY(!harness.manager.isRunning());
    failedSpy.clear();

    // Act: begin and finish a new lifecycle without stderr.
    harness.manager.convert(QStringLiteral("second.pdf"));
    QVERIFY(harness.manager.isRunning());
    harness.processRunner->complete(9, IProcessRunner::ExitStatus::NormalExit);

    // Assert: the failure guard and diagnostic buffer were both reset.
    QCOMPARE(harness.processRunner->startCallCount(), 2);
    compareFailure(failedSpy,
                   ConversionError::NonZeroExit,
                   QStringLiteral("Exit code: 9"));
    QVERIFY(!harness.manager.isRunning());
}

QTEST_APPLESS_MAIN(TestMarkItDownManager)

#include "tst_MarkItDownManager.moc"
