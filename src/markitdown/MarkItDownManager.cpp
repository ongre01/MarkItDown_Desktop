#include "MarkItDownManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QStringList>

namespace {

QString existingFilePath(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString{};
}

QString markItDownRelativeExecutablePath()
{
#ifdef Q_OS_WIN
    return QStringLiteral("Scripts/markitdown.exe");
#else
    return QStringLiteral("bin/markitdown");
#endif
}

QString findApplicationLocalExecutable()
{
    const QDir applicationDirectory(QCoreApplication::applicationDirPath());
    return existingFilePath(
        applicationDirectory.filePath(
            QStringLiteral("python-venv/%1").arg(markItDownRelativeExecutablePath())));
}

QString findDevelopmentEnvironmentExecutable()
{
    const QString relativeExecutablePath = markItDownRelativeExecutablePath();

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

    const QString applicationLocalExecutable = findApplicationLocalExecutable();
    if (!applicationLocalExecutable.isEmpty()) {
        return applicationLocalExecutable;
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
            &MarkItDownManager::collectStandardError);
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
        emit failed(ConversionError::AlreadyRunning, {});
        return;
    }

    m_standardError.clear();
    m_failureReported = false;

    const QString program = resolveMarkItDownExecutable();
    if (program.isEmpty()) {
        const QString configuredExecutable =
            qEnvironmentVariable("MARKITDOWN_EXECUTABLE").trimmed();

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
    m_process->setProcessEnvironment(processEnvironment);

    m_process->start(program, QStringList{filePath});
}

bool MarkItDownManager::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void MarkItDownManager::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    collectStandardError();

    if (m_failureReported) {
        return;
    }

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        const QString markdown = QString::fromUtf8(m_process->readAllStandardOutput());

        if (markdown.trimmed().isEmpty()) {
            reportProcessFailure(ConversionError::EmptyOutput);
            return;
        }

        emit finished(markdown);
        return;
    }

    if (exitStatus == QProcess::CrashExit) {
        reportProcessFailure(ConversionError::Crashed);
        return;
    }

    reportProcessFailure(ConversionError::NonZeroExit,
                         tr("Exit code: %1").arg(exitCode));
}

void MarkItDownManager::processError(QProcess::ProcessError error)
{
    collectStandardError();

    ConversionError conversionError = ConversionError::ProcessFailure;
    switch (error) {
    case QProcess::FailedToStart:
        conversionError = ConversionError::FailedToStart;
        break;
    case QProcess::Crashed:
        conversionError = ConversionError::Crashed;
        break;
    case QProcess::Timedout:
    case QProcess::WriteError:
    case QProcess::ReadError:
    case QProcess::UnknownError:
        break;
    }

    reportProcessFailure(conversionError, m_process->errorString());
}

void MarkItDownManager::collectStandardError()
{
    m_standardError.append(m_process->readAllStandardError());
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
