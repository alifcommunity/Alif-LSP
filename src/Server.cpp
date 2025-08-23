#include "Server.h"
#include "DocManager.h"
#include "Completion.h"
#include "Logger.h"
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

// إرسال رد خطأ وفقاً لمعايير LSP
void LSPServer::sendErrorResponse(const json& id, int code, const std::string& message) {
	json errorResponse = {
		{"jsonrpc", "2.0"},
		{"id", id},
		{"error", {
			{"code", code},
			{"message", message}
		}}
	};
	sendResponse(errorResponse);
	Logger::warn("Error response sent: " + message);
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
	// التحقق من وجود المعاملات المطلوبة
	if (!params.contains("textDocument") || !params.contains("position")) {
		sendErrorResponse(id, -32602, "Missing required parameters: textDocument or position");
		return;
	}
	
	auto textDoc = params["textDocument"];
	auto position = params["position"];
	
	// التحقق من وجود URI في مستند النص
	if (!textDoc.contains("uri") || !textDoc["uri"].is_string()) {
		sendErrorResponse(id, -32602, "Invalid textDocument: missing or invalid uri");
		return;
	}
	
	// التحقق من صحة معاملات الموضع
	if (!position.contains("line") || !position.contains("character") ||
		!position["line"].is_number_integer() || !position["character"].is_number_integer()) {
		sendErrorResponse(id, -32602, "Invalid position: line and character must be integers");
		return;
	}
	
	int line = position["line"].get<int>();
	int character = position["character"].get<int>();
	
	// التحقق من حدود القيم
	if (line < 0 || character < 0) {
		sendErrorResponse(id, -32602, "Invalid position: line and character must be non-negative");
		return;
	}
	
	try {
		json result = completionEngine.getSuggestions(
			textDoc["uri"].get<std::string>(),
			line,
			character
		);
		sendResponse({ {"id", id}, {"result", result} });
		Logger::debug("Completion request processed successfully");
	}
	catch (const std::exception& e) {
		sendErrorResponse(id, -32603, "Internal error: " + std::string(e.what()));
		Logger::error("Completion processing failed: " + std::string(e.what()));
	}
}

void LSPServer::handleMessage(const json& msg) {
	// التحقق من وجود حقل method
	if (!msg.contains("method") || !msg["method"].is_string()) {
		Logger::warn("Received message without valid method field");
		return;
	}
	
	std::string method = msg["method"].get<std::string>();
	Logger::debug("Processing method: " + method);
	
	// معالجة طلب التهيئة
	if (method == "initialize") {
		if (!msg.contains("id")) {
			Logger::warn("Initialize request missing id field");
			return;
		}
		if (!msg.contains("params")) {
			sendErrorResponse(msg["id"], -32602, "Initialize request missing params");
			return;
		}
		initialize(msg["params"]);
	}
	// معالجة فتح مستند
	else if (method == "textDocument/didOpen") {
		if (!msg.contains("params") || !msg["params"].contains("textDocument")) {
			Logger::warn("didOpen request missing required parameters");
			return;
		}
		auto doc = msg["params"]["textDocument"];
		if (!doc.contains("uri") || !doc.contains("text") ||
			!doc["uri"].is_string() || !doc["text"].is_string()) {
			Logger::warn("didOpen request has invalid textDocument structure");
			return;
		}
		docManager.openDocument(doc["uri"], doc["text"]);
		Logger::debug("Document opened: " + doc["uri"].get<std::string>());
	}
	// معالجة تحديث مستند
	else if (method == "textDocument/didChange") {
		if (!msg.contains("params") || !msg["params"].contains("textDocument") ||
			!msg["params"].contains("contentChanges")) {
			Logger::warn("didChange request missing required parameters");
			return;
		}
		auto doc = msg["params"]["textDocument"];
		auto changes = msg["params"]["contentChanges"];
		
		if (!doc.contains("uri") || !doc["uri"].is_string() ||
			!changes.is_array() || changes.empty()) {
			Logger::warn("didChange request has invalid structure");
			return;
		}
		
		if (!changes[0].contains("text") || !changes[0]["text"].is_string()) {
			Logger::warn("didChange request has invalid content changes");
			return;
		}
		
		docManager.updateDocument(doc["uri"], changes[0]["text"]);
		Logger::debug("Document updated: " + doc["uri"].get<std::string>());
	}
	// معالجة طلب الإكمال التلقائي
	else if (method == "textDocument/completion") {
		if (!msg.contains("id")) {
			Logger::warn("Completion request missing id field");
			return;
		}
		if (!msg.contains("params")) {
			sendErrorResponse(msg["id"], -32602, "Completion request missing params");
			return;
		}
		handleCompletion(msg["params"], msg["id"]);
	}
	// طرق غير مدعومة
	else {
		Logger::debug("Unsupported method: " + method);
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

	Logger::info("Alif Server Started");

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
			Logger::error("Error reading input");
			return -1;
		}
		// هنا نقوم بتحليل الرسالة الواردة
		try {
			json msg = json::parse(buffer.begin(), buffer.end());
			// ممكن نعمل log للرسالة المستلمة لأغراض التصحيح
			// std::cerr << "[Alif-LSP] Received: " << msg.dump(2) << std::endl;
			handleMessage(msg);
		}
		catch (const std::exception& e) {

			Logger::error("JSON Parse Error: " + std::string(e.what()));
		}
	}

	return 0;
}
