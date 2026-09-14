#ifndef MARKITDOWNMANAGER_H
#define MARKITDOWNMANAGER_H

#include "IMarkItDownManager.h"
#include "ProcessRunner.h"

#include <QByteArray>

#include <memory>

class IMarkItDownExecutableResolver;

class MarkItDownManager : public IMarkItDownManager
{
    Q_OBJECT

public:
    explicit MarkItDownManager(QObject *parent = nullptr);
    ~MarkItDownManager() override;

    MarkItDownManager(
        std::unique_ptr<IProcessRunner> processRunner,
        std::unique_ptr<IMarkItDownExecutableResolver> executableResolver,
        QObject *parent = nullptr);

    void convert(const QString &filePath) override;

    bool isRunning() const override;

private slots:
    void processFinished(int exitCode, IProcessRunner::ExitStatus exitStatus);

    void processError(IProcessRunner::ProcessError error);

    void collectStandardError();

private:
    void reportProcessFailure(ConversionError error,
                              const QString &processDetails = {});
    QString diagnosticDetails(const QString &processDetails = {}) const;
    QString decodedStandardError() const;

    const std::unique_ptr<IProcessRunner> m_processRunner;
    const std::unique_ptr<IMarkItDownExecutableResolver> m_executableResolver;
    QByteArray m_standardError;
    bool m_conversionActive = false;
    bool m_failureReported = false;
};

#endif // MARKITDOWNMANAGER_H
