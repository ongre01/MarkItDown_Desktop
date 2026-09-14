#include "MarkItDownManager.h"

#include "MarkItDownExecutableResolver.h"

#include <QDir>
#include <QProcessEnvironment>
#include <QStringList>

#include <utility>

MarkItDownManager::MarkItDownManager(QObject *parent)
    : MarkItDownManager(std::make_unique<QProcessRunner>(),
                        std::make_unique<MarkItDownExecutableResolver>(),
                        parent)
{
}

MarkItDownManager::~MarkItDownManager() = default;

MarkItDownManager::MarkItDownManager(
    std::unique_ptr<IProcessRunner> processRunner,
    std::unique_ptr<IMarkItDownExecutableResolver> executableResolver,
    QObject *parent)
    : IMarkItDownManager(parent)
    , m_processRunner(std::move(processRunner))
    , m_executableResolver(std::move(executableResolver))
{
    Q_ASSERT(m_processRunner);
    Q_ASSERT(m_executableResolver);

    connect(m_processRunner.get(),
            &IProcessRunner::started,
            this,
            &MarkItDownManager::started);
    connect(m_processRunner.get(),
            &IProcessRunner::readyReadStandardError,
            this,
            &MarkItDownManager::collectStandardError);
    connect(m_processRunner.get(),
            &IProcessRunner::errorOccurred,
            this,
            &MarkItDownManager::processError);
    connect(m_processRunner.get(),
            &IProcessRunner::finished,
            this,
            &MarkItDownManager::processFinished);
}

void MarkItDownManager::convert(const QString &filePath)
{
    if (isRunning()) {
        emit failed(ConversionError::AlreadyRunning, {});
        return;
    }

    m_standardError.clear();
    m_failureReported = false;

    const QString program = m_executableResolver->resolve();
    if (program.isEmpty()) {
        const QString configuredExecutable = m_executableResolver->configuredExecutable();

        if (!configuredExecutable.isEmpty()) {
            emit failed(ConversionError::ExecutableNotFound,
                        tr("MARKITDOWN_EXECUTABLE: %1")
                            .arg(QDir::toNativeSeparators(configuredExecutable)));
        } else {
            emit failed(ConversionError::ExecutableNotFound, {});
        }
        return;
    }

    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    processEnvironment.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    processEnvironment.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    m_processRunner->setProcessEnvironment(processEnvironment);

    m_processRunner->start(program, QStringList{filePath});
}

bool MarkItDownManager::isRunning() const
{
    return m_processRunner->isRunning();
}

void MarkItDownManager::processFinished(int exitCode,
                                        IProcessRunner::ExitStatus exitStatus)
{
    collectStandardError();

    if (m_failureReported) {
        return;
    }

    if (exitStatus == IProcessRunner::ExitStatus::NormalExit && exitCode == 0) {
        const QString markdown =
            QString::fromUtf8(m_processRunner->readAllStandardOutput());

        if (markdown.trimmed().isEmpty()) {
            reportProcessFailure(ConversionError::EmptyOutput);
            return;
        }

        emit finished(markdown);
        return;
    }

    if (exitStatus == IProcessRunner::ExitStatus::CrashExit) {
        reportProcessFailure(ConversionError::Crashed);
        return;
    }

    reportProcessFailure(ConversionError::NonZeroExit,
                         tr("Exit code: %1").arg(exitCode));
}

void MarkItDownManager::processError(IProcessRunner::ProcessError error)
{
    collectStandardError();

    ConversionError conversionError = ConversionError::ProcessFailure;
    switch (error) {
    case IProcessRunner::ProcessError::FailedToStart:
        conversionError = ConversionError::FailedToStart;
        break;
    case IProcessRunner::ProcessError::Crashed:
        conversionError = ConversionError::Crashed;
        break;
    case IProcessRunner::ProcessError::Timedout:
    case IProcessRunner::ProcessError::WriteError:
    case IProcessRunner::ProcessError::ReadError:
    case IProcessRunner::ProcessError::UnknownError:
        break;
    }

    reportProcessFailure(conversionError, m_processRunner->errorString());
}

void MarkItDownManager::collectStandardError()
{
    m_standardError.append(m_processRunner->readAllStandardError());
}

void MarkItDownManager::reportProcessFailure(ConversionError error,
                                             const QString &processDetails)
{
    if (m_failureReported) {
        return;
    }

    m_failureReported = true;
    emit failed(error, diagnosticDetails(processDetails));
}

QString MarkItDownManager::decodedStandardError() const
{
    return QString::fromUtf8(m_standardError).trimmed();
}

QString MarkItDownManager::diagnosticDetails(const QString &processDetails) const
{
    QStringList details;

    const QString normalizedProcessDetails = processDetails.trimmed();
    if (!normalizedProcessDetails.isEmpty()) {
        details.append(normalizedProcessDetails);
    }

    const QString standardError = decodedStandardError();
    if (!standardError.isEmpty()) {
        details.append(tr("Standard error:\n%1").arg(standardError));
    }

    return details.join(QStringLiteral("\n\n"));
}
