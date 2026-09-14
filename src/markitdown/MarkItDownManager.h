#ifndef MARKITDOWNMANAGER_H
#define MARKITDOWNMANAGER_H

#include "../model/ConversionError.h"

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>

class MarkItDownManager : public QObject
{
    Q_OBJECT

public:
    explicit MarkItDownManager(QObject *parent = nullptr);

    void convert(const QString &filePath);

    bool isRunning() const;

signals:
    void started();

    void finished(const QString &markdown);

    void failed(ConversionError error, const QString &details);

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void processError(QProcess::ProcessError error);

    void collectStandardError();

private:
    void reportProcessFailure(ConversionError error,
                              const QString &processDetails = {});
    QString diagnosticDetails(const QString &processDetails = {}) const;
    QString decodedStandardError() const;

    QProcess *const m_process;
    QByteArray m_standardError;
    bool m_failureReported = false;
};

#endif // MARKITDOWNMANAGER_H
