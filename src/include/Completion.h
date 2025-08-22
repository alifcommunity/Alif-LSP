#pragma once
#include <vector>
#include <string>
#include "json.hpp"
#include "Context.h"

using json = nlohmann::json;

class Completion {
public:
	json getSuggestions(const std::string& uri, int line, int character);
	std::string getCurrentWord(const std::string& uri, int line, int character);

private:
	ContextAnalyzer contextAnalyzer;
	std::vector<std::string> getContextualKeywords(const CodeContext& context);
};