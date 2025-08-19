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
			{"triggerCharacters", {".", ":"}}
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
	if (msg.contains("id") && msg["method"] == "initialize") {
		initialize(msg["params"]);
	}
	else if (msg["method"] == "textDocument/didOpen") {
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

void LSPServer::run() {
	// Set binary mode for stdin/stdout
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);

	std::cerr << "[Alif-LSP] Alif Server Started" << std::endl;

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
			std::cerr << "[Alif-LSP] Error reading input" << std::endl;
			return;
		}
		// هنا نقوم بتحليل الرسالة الواردة
		try {
			json msg = json::parse(buffer.begin(), buffer.end());
			// ممكن نعمل log للرسالة المستلمة لأغراض التصحيح
			// std::cerr << "[Alif-LSP] Received: " << msg.dump(2) << std::endl;
			handleMessage(msg);
		}
		catch (const std::exception& e) {

			std::cerr << "[Alif-LSP] JSON Parse Error: " << e.what() << std::endl;
		}
	}
}