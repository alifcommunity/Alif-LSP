#pragma once
#include <vector>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class Completion {
public:
	json getSuggestions();
};
