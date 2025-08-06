#pragma once
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class LSPServer {
public:
	void run();
	void handleMessage(const json& msg);

private:
	void sendResponse(const json& response);
	void initialize(const json& params);
	void handleCompletion(const json& params, const json& id);
};