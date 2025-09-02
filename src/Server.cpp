#include "Server.h"
#include "DocManager.h"
#include "Completion.h"
#include "Logger.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#if defined(_WIN32)
#define NOMINMAX
#include <io.h>
#include <windows.h>
#endif
#include <algorithm>
#include <stdio.h>
#include <fcntl.h>


DocumentManager docManager;
Completion completionEngine;

void LSPServer::sendResponse(const json& response) {
	try {
		std::string str = response.dump();
		std::cout << "Content-Length: " << str.size() << "\r\n\r\n" << str;
		std::cout.flush();
		
		// Check if output operation was successful
		if (std::cout.fail()) {
			Logger::error("Failed to send response - output stream error");
		} else {
			Logger::debug("Response sent successfully (" + std::to_string(str.size()) + " bytes)");
		}
	}
	catch (const std::exception& e) {
		Logger::error("Error sending response: " + std::string(e.what()));
	}
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

void LSPServer::initialize(const json& params, const json& requestId) {
	json capabilities = {
		{"completionProvider", {
			{"triggerCharacters", true}
		}},
		{"textDocumentSync", 1} // Full sync
	};

	sendResponse({
		{"jsonrpc", "2.0"},
		{"id", requestId},
		{"result", {
			{"capabilities", capabilities}
		}}
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

	std::string uri = textDoc["uri"].get<std::string>();

	// التحقق من وجود المستند في DocumentManager
	if (!docManager.hasDocument(uri)) {
		Logger::warn("Completion requested for unopened document: " + uri);
		sendErrorResponse(id, -32603, "Document not found: " + uri);
		return;
	}

	try {
		json result = completionEngine.getSuggestions(uri, line, character);
		sendResponse({ 
			{"jsonrpc", "2.0"},
			{"id", id}, 
			{"result", result} 
		});
		Logger::debug("Completion request processed successfully for: " + uri);
	}
	catch (const std::exception& e) {
		sendErrorResponse(id, -32603, "Internal error: " + std::string(e.what()));
		Logger::error("Completion processing failed for " + uri + ": " + std::string(e.what()));
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
		initialize(msg["params"], msg["id"]);
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
		DocumentError result = docManager.openDocument(doc["uri"], doc["text"]);
		if (result != DocumentError::SUCCESS) {
			Logger::warn("Failed to open document " + doc["uri"].get<std::string>() +
				": " + DocumentManager::errorToString(result));
		}
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
			!changes.is_array()) {
			Logger::warn("didChange request has invalid structure");
			return;
		}

		// Handle empty changes array (valid LSP - means no changes)
		if (changes.empty()) {
			Logger::debug("Received didChange with empty contentChanges array - no changes to apply");
			return;
		}

		if (!changes[0].contains("text") || !changes[0]["text"].is_string()) {
			Logger::warn("didChange request has invalid content changes");
			return;
		}

		DocumentError result = docManager.updateDocument(doc["uri"], changes[0]["text"]);
		if (result != DocumentError::SUCCESS) {
			Logger::warn("Failed to update document " + doc["uri"].get<std::string>() +
				": " + DocumentManager::errorToString(result));
		}
	}
	// معالجة إغلاق مستند
	else if (method == "textDocument/didClose") {
		if (!msg.contains("params") || !msg["params"].contains("textDocument")) {
			Logger::warn("didClose request missing required parameters");
			return;
		}
		auto doc = msg["params"]["textDocument"];
		if (!doc.contains("uri") || !doc["uri"].is_string()) {
			Logger::warn("didClose request has invalid textDocument structure");
			return;
		}
		DocumentError result = docManager.closeDocument(doc["uri"]);
		if (result != DocumentError::SUCCESS) {
			Logger::warn("Failed to close document " + doc["uri"].get<std::string>() +
				": " + DocumentManager::errorToString(result));
		}
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

// التحقق من صحة بنية رسالة LSP الأساسية
bool LSPServer::isValidLSPMessage(const json& msg) {
	// التحقق من أن الرسالة كائن JSON
	if (!msg.is_object()) {
		Logger::debug("Message validation failed: not a JSON object");
		return false;
	}

	// التحقق من وجود حقل jsonrpc (اختياري لكن مفيد)
	if (msg.contains("jsonrpc")) {
		if (!msg["jsonrpc"].is_string() || msg["jsonrpc"] != "2.0") {
			Logger::debug("Message validation failed: invalid jsonrpc version");
			return false;
		}
	}

	// التحقق من وجود حقل method
	if (!msg.contains("method")) {
		Logger::debug("Message validation failed: missing method field");
		return false;
	}

	if (!msg["method"].is_string()) {
		Logger::debug("Message validation failed: method is not a string");
		return false;
	}

	// التحقق من أن method ليس فارغاً
	std::string method = msg["method"].get<std::string>();
	if (method.empty()) {
		Logger::debug("Message validation failed: empty method");
		return false;
	}

	// التحقق من أن id موجود وصالح للطلبات التي تحتاج رد
	if (msg.contains("id")) {
		if (!msg["id"].is_number() && !msg["id"].is_string() && !msg["id"].is_null()) {
			Logger::debug("Message validation failed: invalid id type");
			return false;
		}
	}

	Logger::debug("Message validation passed for method: " + method);
	return true;
}

int LSPServer::run() {
#if defined(_WIN32)
	// Set binary mode for stdin/stdout on Windows
	int stdinInit = _setmode(_fileno(stdin), _O_BINARY);
	int stdoutInit = _setmode(_fileno(stdout), _O_BINARY);
	if (stdinInit == -1 || stdoutInit == -1) {
		Logger::error("Fatal Error: Cannot set binary mode for stdin/stdout");
		perror("Cannot set mode");
		return -1;
	}
#endif

	Logger::info("Alif Server Started");

	while (true) {
		// قراءة رؤوس الرسالة مع التحقق من الصحة
		std::string line;
		size_t length = 0;
		bool contentLengthFound = false;

		// قراءة الرؤوس حتى الوصول لسطر فارغ
		while (std::getline(std::cin, line)) {
			// إزالة الأحرف الإضافية من نهاية السطر
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			// البحث عن رأس Content-Length
			if (line.find("Content-Length: ") == 0) {
				std::string lengthStr = line.substr(16);

				// التحقق من صحة القيمة الرقمية
				try {
					length = std::stoul(lengthStr);
					contentLengthFound = true;

					// التحقق من الحدود المعقولة للرسالة
					if (length > 1024 * 1024) { // حد أقصى 1MB
						Logger::warn("Message too large: " + std::to_string(length) + " bytes");
						continue; // تجاهل الرسالة الكبيرة جداً
					}
					if (length == 0) {
						Logger::warn("Empty message received (Content-Length: 0)");
						continue;
					}

					Logger::debug("Content-Length: " + std::to_string(length));
				}
				catch (const std::exception& e) {
					Logger::warn("Invalid Content-Length header: " + lengthStr + " - " + std::string(e.what()));
					continue; // تجاهل الرسالة مع رأس خاطئ
				}
			}

			// نهاية الرؤوس
			if (line.empty()) {
				break;
			}
		}

		// التحقق من وجود رأس Content-Length
		if (!contentLengthFound) {
			// فحص حالة انقطاع الاتصال من العميل
			if (std::cin.eof()) {
				Logger::info("Client disconnected (EOF), shutting down server");
				break; // خروج من الحلقة الرئيسية
			}
			if (std::cin.fail()) {
				Logger::warn("Input stream error, shutting down server");
				break; // خروج من الحلقة الرئيسية
			}

			// رسالة غير صحيحة - تسجيل تحذير والمتابعة
			Logger::warn("Message received without Content-Length header");
			continue;
		}

		// قراءة محتوى الرسالة مع الحماية من الأخطاء
		std::vector<char> buffer(length);
		std::cin.read(buffer.data(), length);

		// التحقق من نجاح القراءة
		if (!std::cin) {
			Logger::error("Failed to read message body (" + std::to_string(length) + " bytes)");
			// محاولة استعادة الحالة
			std::cin.clear();
			continue;
		}

		// التحقق من قراءة البيانات كاملة
		size_t bytesRead = std::cin.gcount();
		if (bytesRead != length) {
			Logger::warn("Incomplete message read: expected " + std::to_string(length) +
				" bytes, got " + std::to_string(bytesRead));
			continue;
		}

		// تحليل JSON مع معالجة محسنة للأخطاء
		try {
			// التحقق من أن البيانات ليست فارغة
			if (buffer.empty()) {
				Logger::warn("Empty message buffer received");
				continue;
			}

			// تحليل JSON
			json msg = json::parse(buffer.begin(), buffer.end());

			// التحقق من أن الرسالة تحتوي على حقول أساسية
			if (msg.is_null() || (!msg.is_object())) {
				Logger::warn("Invalid JSON structure: not an object");
				continue;
			}

			// التحقق من صحة رسالة LSP
			if (!isValidLSPMessage(msg)) {
				Logger::warn("Invalid LSP message structure received");
				continue;
			}

			Logger::debug("Message parsed successfully");
			handleMessage(msg);
		}
		catch (const json::parse_error& e) {
			// خطأ في تحليل JSON مع تفاصيل أكثر
			Logger::error("JSON Parse Error at byte " + std::to_string(e.byte) +
				": " + std::string(e.what()));

			// إظهار جزء من البيانات المشكوكة للتشخيص
			std::string preview(buffer.begin(), buffer.begin() + std::min(size_t(100), buffer.size()));
			Logger::debug("Message preview: " + preview + (buffer.size() > 100 ? "..." : ""));
		}
		catch (const std::exception& e) {
			Logger::error("Unexpected error during message processing: " + std::string(e.what()));
		}
	}

	return 0;
}
