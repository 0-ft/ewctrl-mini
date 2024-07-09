#ifndef BEZIERJSONPARSER_H
#define BEZIERJSONPARSER_H

#include <map>
#include <string>
#include <ArduinoJson.h>
#include "BezierPattern.h"

std::map<std::string, BezierPattern> parseJsonToBezierPatterns(const JsonArray& doc);

#endif // BEZIERJSONPARSER_H
