#ifndef PROCESSRUNNER_H
#define PROCESSRUNNER_H

#include <QByteArray>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

class IProcessRunner : public QObject
{
    Q_OBJECT

public:
    enum class ExitStatus
    {
        NormalExit,
        CrashExit
    };
    Q_ENUM(ExitStatus)

    enum class ProcessError
    {
        FailedToStart,
        Crashed,
        Timedout,
        WriteError,
        ReadError,
        UnknownError
    };
    Q_ENUM(ProcessError)

    explicit IProcessRunner(QObject *parent = nullptr);
    ~IProcessRunner() override = default;

    virtual bool isRunning() const = 0;
    virtual void setProcessEnvironment(const QProcessEnvironment &environment) = 0;
    virtual void start(const QString &program, const QStringList &arguments) = 0;
    virtual QByteArray readAllStandardOutput() = 0;
    virtual QByteArray readAllStandardError() = 0;
    virtual QString errorString() const = 0;

signals:
    void started();
    void readyReadStandardError();
    void errorOccurred(IProcessRunner::ProcessError error);
    void finished(int exitCode, IProcessRunner::ExitStatus exitStatus);
};

class QProcess;

class QProcessRunner final : public IProcessRunner
{
public:
    explicit QProcessRunner(QObject *parent = nullptr);

    bool isRunning() const override;
    void setProcessEnvironment(const QProcessEnvironment &environment) override;
    void start(const QString &program, const QStringList &arguments) override;
    QByteArray readAllStandardOutput() override;
    QByteArray readAllStandardError() override;
    QString errorString() const override;

private:
    QProcess *const m_process;
};

#endif // PROCESSRUNNER_H
