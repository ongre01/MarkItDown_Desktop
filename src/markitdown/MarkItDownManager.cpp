#include "MarkItDownManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

namespace {

QString existingFilePath(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString{};
}

QString findDevelopmentEnvironmentExecutable()
{
#ifdef Q_OS_WIN
    const QString relativeExecutablePath = QStringLiteral("Scripts/markitdown.exe");
#else
    const QString relativeExecutablePath = QStringLiteral("bin/markitdown");
#endif

    QStringList searchRoots{QCoreApplication::applicationDirPath(), QDir::currentPath()};
    QStringList visitedDirectories;

    for (const QString &searchRoot : searchRoots) {
        QDir directory(searchRoot);

        do {
            const QString absoluteDirectory = directory.absolutePath();
            if (visitedDirectories.contains(absoluteDirectory, Qt::CaseInsensitive)) {
                continue;
            }

            visitedDirectories.append(absoluteDirectory);

            const QStringList candidates{
                directory.filePath(QStringLiteral("python-venv/%1").arg(relativeExecutablePath)),
                directory.filePath(QStringLiteral("build/python-venv/%1").arg(relativeExecutablePath))};

            for (const QString &candidate : candidates) {
                const QString executable = existingFilePath(candidate);
                if (!executable.isEmpty()) {
                    return executable;
                }
            }
        } while (directory.cdUp());
    }

    return {};
}

QString resolveMarkItDownExecutable()
{
    const QString configuredExecutable =
        qEnvironmentVariable("MARKITDOWN_EXECUTABLE").trimmed();

    if (!configuredExecutable.isEmpty()) {
        const QString configuredFile = existingFilePath(configuredExecutable);
        if (!configuredFile.isEmpty()) {
            return configuredFile;
        }

        return QStandardPaths::findExecutable(configuredExecutable);
    }

    const QString pathExecutable =
        QStandardPaths::findExecutable(QStringLiteral("markitdown"));
    if (!pathExecutable.isEmpty()) {
        return pathExecutable;
    }

    return findDevelopmentEnvironmentExecutable();
}

} // namespace

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

    const QString program = resolveMarkItDownExecutable();
    if (program.isEmpty()) {
        const QString configuredExecutable =
            qEnvironmentVariable("MARKITDOWN_EXECUTABLE").trimmed();

        if (!configuredExecutable.isEmpty()) {
            emit failed(tr("The MarkItDown executable configured by "
                           "MARKITDOWN_EXECUTABLE was not found: %1")
                            .arg(configuredExecutable));
        } else {
            emit failed(tr("MarkItDown CLI was not found. Install MarkItDown, add it to "
                           "PATH, or set MARKITDOWN_EXECUTABLE."));
        }
        return;
    }

    m_process->start(program, QStringList{filePath});
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
