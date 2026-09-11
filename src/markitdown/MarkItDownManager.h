#ifndef MARKITDOWNMANAGER_H
#define MARKITDOWNMANAGER_H

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

    void failed(const QString &message);

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void processError(QProcess::ProcessError error);

    void readStandardError();

private:
    QString standardErrorMessage() const;

    QProcess *m_process;
    QByteArray m_standardError;
    bool m_failureReported = false;
};

#endif // MARKITDOWNMANAGER_H
