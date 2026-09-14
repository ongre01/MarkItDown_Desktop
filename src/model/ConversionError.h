#ifndef CONVERSIONERROR_H
#define CONVERSIONERROR_H

#include <QMetaType>

enum class ConversionError
{
    AlreadyRunning,
    ExecutableNotFound,
    FailedToStart,
    Crashed,
    NonZeroExit,
    EmptyOutput,
    ProcessFailure
};

Q_DECLARE_METATYPE(ConversionError)

#endif // CONVERSIONERROR_H
