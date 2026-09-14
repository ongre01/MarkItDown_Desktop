#ifndef CONVERSIONERRORPRESENTATION_H
#define CONVERSIONERRORPRESENTATION_H

#include "../model/ConversionError.h"

#include <QString>

struct ConversionErrorPresentation
{
    QString title;
    QString summary;
};

ConversionErrorPresentation conversionErrorPresentation(ConversionError error);

#endif // CONVERSIONERRORPRESENTATION_H
