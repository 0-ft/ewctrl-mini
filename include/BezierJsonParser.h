#ifndef BEZIERJSONPARSER_H
#define BEZIERJSONPARSER_H

#include <vector>
#include <ArduinoJson.h>
#include "BezierPattern.h"

std::vector<BezierPattern> parseJsonToBezierPatterns(const String& jsonString);

#endif // BEZIERJSONPARSER_H
