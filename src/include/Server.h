#pragma once
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class LSPServer {
public:
	int run();
	void handleMessage(const json& msg);

private:
	void sendResponse(const json& response);
	void sendNotification(const json& notification);
	// 1: Error, 2: Warning, 3: Info, 4: Log
	void logMessage(const std::string& message, int type = 3);
	void initialize(const json& params);
	void handleCompletion(const json& params, const json& id);
};
