#pragma once
#include <string>
#include "json.hpp"
#include "Logger.h"

using json = nlohmann::json;

class LSPServer {
public:
	int run();
	void handleMessage(const json& msg);

private:
	void sendResponse(const json& response);
	void sendErrorResponse(const json& id, int code, const std::string& message);
	void initialize(const json& params);
	void handleCompletion(const json& params, const json& id);
	bool isValidLSPMessage(const json& msg);
};
