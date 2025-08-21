#include "Server.h"
#include "DocManager.h"
#include "Completion.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <windows.h>

#include <stdio.h>
#include <fcntl.h>
#include <io.h>


DocumentManager docManager;
Completion completionEngine;

void LSPServer::sendResponse(const json& response) {
	std::string str = response.dump();
	std::cout << "Content-Length: " << str.size() << "\r\n\r\n" << str;
	std::cout.flush();
}

void LSPServer::initialize(const json& params) {
	json capabilities = {
		{"completionProvider", {
			{"triggerCharacters", true}
		}},
		{"textDocumentSync", 1} // Full sync
	};

	sendResponse({
		{"id", 1},
			{"result", {
				{"capabilities", capabilities}
			}},
		});
}

void LSPServer::handleCompletion(const json& params, const json& id) {
	auto textDoc = params["textDocument"];
	auto position = params["position"];
	json result = completionEngine.getSuggestions(
		textDoc["uri"].get<std::string>(),
		position["line"].get<int>(),
		position["character"].get<int>()
	);
	sendResponse({ {"id", id}, {"result", result} });
}

void LSPServer::handleMessage(const json& msg) {
	if (msg["method"] == "textDocument/didOpen") {
		auto doc = msg["params"]["textDocument"];
		docManager.openDocument(doc["uri"], doc["text"]);
	}
	else if (msg["method"] == "textDocument/didChange") {
		auto doc = msg["params"]["textDocument"];
		auto changes = msg["params"]["contentChanges"];
		docManager.updateDocument(doc["uri"], changes[0]["text"]);
	}
	else if (msg["method"] == "textDocument/completion") {
		handleCompletion(msg["params"], msg["id"]);
	}
}

int LSPServer::run() {
	// Set binary mode for stdin/stdout
	int stdinInit = _setmode(_fileno(stdin), _O_BINARY);
	int stdoutInit = _setmode(_fileno(stdout), _O_BINARY);
	if (stdinInit == -1 or stdoutInit == -1) {
		perror("Fatal Error: Cannot set mode");
		return -1;
	}

	std::cerr << "[Alif-LSP] Alif Server Started" << std::endl;

	// Handle initialization handshake before main loop
	bool initialized = false;
	while (!initialized) {
		// Read Content-Length header
		std::string line;
		size_t length = 0;
		while (std::getline(std::cin, line)) {
			if (line.find("Content-Length: ") == 0) {
				length = std::stoul(line.substr(16));
			}
			if (line == "\r") break;
		}

		// Read JSON body
		std::vector<char> buffer(length);
		std::cin.read(buffer.data(), length);

		// تأكد من قراءة كل البيانات
		if (!std::cin) {
			return -1;
		}

		try {
			json msg = json::parse(buffer.begin(), buffer.end());
			if (msg.contains("id") && msg["method"] == "initialize") {
				initialize(msg["params"]);
			}
			else if (msg.contains("method") && msg["method"] == "initialized") {
				initialized = true;
				std::cerr << "[Alif-LSP] Initialization complete" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[Alif-LSP] JSON Parse Error during initialization: " << e.what() << std::endl;
		}
	}

	// Main message loop
	while (true) {
		// Read Content-Length header
		std::string line;
		size_t length = 0;
		while (std::getline(std::cin, line)) {
			if (line.find("Content-Length: ") == 0) {
				length = std::stoul(line.substr(16));
			}
			if (line == "\r") break;
		}

		// Read JSON body
		std::vector<char> buffer(length);
		std::cin.read(buffer.data(), length);

		// تأكد من قراءة كل البيانات
		if (!std::cin) {
			return -1;
		}
		// هنا نقوم بتحليل الرسالة الواردة
		try {
			json msg = json::parse(buffer.begin(), buffer.end());

			handleMessage(msg);
		}
		catch (const std::exception& e) {
			std::cerr << "[Alif-LSP] JSON Parse Error: " << e.what() << std::endl;
		}
	}

	return 0;
}
