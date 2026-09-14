#include "MarkItDownExecutableResolver.h"

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

} // namespace

QString MarkItDownExecutableResolver::resolve() const
{
    const QString configured = configuredExecutable();

    if (!configured.isEmpty()) {
        const QString configuredFile = existingFilePath(configured);
        if (!configuredFile.isEmpty()) {
            return configuredFile;
        }

        return QStandardPaths::findExecutable(configured);
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

QString MarkItDownExecutableResolver::configuredExecutable() const
{
    return qEnvironmentVariable("MARKITDOWN_EXECUTABLE").trimmed();
}
