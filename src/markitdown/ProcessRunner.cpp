#include "ProcessRunner.h"

#include <QProcess>

namespace {

IProcessRunner::ExitStatus processExitStatus(QProcess::ExitStatus exitStatus)
{
    return exitStatus == QProcess::NormalExit
               ? IProcessRunner::ExitStatus::NormalExit
               : IProcessRunner::ExitStatus::CrashExit;
}

IProcessRunner::ProcessError processError(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        return IProcessRunner::ProcessError::FailedToStart;
    case QProcess::Crashed:
        return IProcessRunner::ProcessError::Crashed;
    case QProcess::Timedout:
        return IProcessRunner::ProcessError::Timedout;
    case QProcess::WriteError:
        return IProcessRunner::ProcessError::WriteError;
    case QProcess::ReadError:
        return IProcessRunner::ProcessError::ReadError;
    case QProcess::UnknownError:
        return IProcessRunner::ProcessError::UnknownError;
    }

    return IProcessRunner::ProcessError::UnknownError;
}

} // namespace

IProcessRunner::IProcessRunner(QObject *parent)
    : QObject(parent)
{
}

QProcessRunner::QProcessRunner(QObject *parent)
    : IProcessRunner(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, &IProcessRunner::started);
    connect(m_process,
            &QProcess::readyReadStandardError,
            this,
            &IProcessRunner::readyReadStandardError);
    connect(m_process,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                emit errorOccurred(processError(error));
            });
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                emit finished(exitCode, processExitStatus(exitStatus));
            });
}

bool QProcessRunner::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void QProcessRunner::setProcessEnvironment(const QProcessEnvironment &environment)
{
    m_process->setProcessEnvironment(environment);
}

void QProcessRunner::start(const QString &program, const QStringList &arguments)
{
    m_process->start(program, arguments);
}

QByteArray QProcessRunner::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

QByteArray QProcessRunner::readAllStandardError()
{
    return m_process->readAllStandardError();
}

QString QProcessRunner::errorString() const
{
    return m_process->errorString();
}
