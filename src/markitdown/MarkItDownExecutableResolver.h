#ifndef MARKITDOWNEXECUTABLERESOLVER_H
#define MARKITDOWNEXECUTABLERESOLVER_H

#include <QString>

class IMarkItDownExecutableResolver
{
public:
    virtual ~IMarkItDownExecutableResolver() = default;

    virtual QString resolve() const = 0;
    virtual QString configuredExecutable() const = 0;
};

class MarkItDownExecutableResolver final : public IMarkItDownExecutableResolver
{
public:
    QString resolve() const override;
    QString configuredExecutable() const override;
};

#endif // MARKITDOWNEXECUTABLERESOLVER_H
