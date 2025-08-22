#pragma once
#include <vector>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class Completion {
public:
	json getSuggestions(const std::string& uri, int line, int character);
	std::string getCurrentWord(const std::string& uri, int line, int character);
};