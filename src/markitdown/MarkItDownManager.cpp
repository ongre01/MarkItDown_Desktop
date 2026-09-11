#include "MarkItDownManager.h"

#include <QStringList>

MarkItDownManager::MarkItDownManager(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, &MarkItDownManager::started);
    connect(m_process,
            &QProcess::readyReadStandardError,
            this,
            &MarkItDownManager::readStandardError);
    connect(m_process,
            &QProcess::errorOccurred,
            this,
            &MarkItDownManager::processError);
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &MarkItDownManager::processFinished);
}

void MarkItDownManager::convert(const QString &filePath)
{
    if (isRunning()) {
        emit failed(tr("Another conversion is already running."));
        return;
    }

    m_standardError.clear();
    m_failureReported = false;

    m_process->start(QStringLiteral("markitdown"), QStringList{filePath});
}

bool MarkItDownManager::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void MarkItDownManager::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    readStandardError();

    if (m_failureReported) {
        return;
    }

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        const QString markdown = QString::fromUtf8(m_process->readAllStandardOutput());
        emit finished(markdown);
        return;
    }

    m_failureReported = true;

    QString message = standardErrorMessage();
    if (message.isEmpty()) {
        if (exitStatus == QProcess::CrashExit) {
            message = tr("MarkItDown terminated unexpectedly.");
        } else {
            message = tr("MarkItDown exited with code %1.").arg(exitCode);
        }
    }

    emit failed(message);
}

void MarkItDownManager::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error)

    readStandardError();

    if (m_failureReported) {
        return;
    }

    m_failureReported = true;

    QString message = standardErrorMessage();
    if (message.isEmpty()) {
        message = m_process->errorString();
    }

    if (message.isEmpty()) {
        message = tr("MarkItDown execution failed.");
    }

    emit failed(message);
}

void MarkItDownManager::readStandardError()
{
    m_standardError.append(m_process->readAllStandardError());
}

QString MarkItDownManager::standardErrorMessage() const
{
    return QString::fromUtf8(m_standardError).trimmed();
}
