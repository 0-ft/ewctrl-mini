#ifndef BEZIERJSONPARSER_H
#define BEZIERJSONPARSER_H

#include <map>
#include <string>
#include <ArduinoJson.h>
#include "BezierPattern.h"
#include <esp_log.h>

std::pair<std::string, BezierPattern> parseJsonToBezierPattern(const JsonObject &patternObj);
std::map<std::string, BezierPattern> parseJsonToBezierPatterns(const JsonArray& doc);

#endif // BEZIERJSONPARSER_H
