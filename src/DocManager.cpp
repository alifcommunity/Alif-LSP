#include "DocManager.h"
#include "Logger.h"

// فتح مستند جديد مع التحقق من الصحة
DocumentError DocumentManager::openDocument(const std::string& uri, const std::string& text) {
	// التحقق من صحة URI
	if (!isValidURI(uri)) {
		Logger::warn("Attempt to open document with invalid URI: " + uri);
		return DocumentError::INVALID_URI;
	}

	// التحقق من عدم وجود المستند مسبقاً
	if (documents.find(uri) != documents.end()) {
		Logger::warn("Attempt to open already existing document: " + uri);
		return DocumentError::DOCUMENT_ALREADY_EXISTS;
	}

	// حفظ المستند
	try {
		documents[uri] = text;
		Logger::info("Document opened successfully: " + uri + " (" + std::to_string(text.length()) + " chars)");
		return DocumentError::SUCCESS;
	}
	catch (const std::exception& e) {
		Logger::error("Failed to open document " + uri + ": " + std::string(e.what()));
		return DocumentError::OPERATION_FAILED;
	}
}

// تحديث مستند موجود
DocumentError DocumentManager::updateDocument(const std::string& uri, const std::string& text) {
	// التحقق من صحة URI
	if (!isValidURI(uri)) {
		Logger::warn("Attempt to update document with invalid URI: " + uri);
		return DocumentError::INVALID_URI;
	}

	// التحقق من وجود المستند
	auto it = documents.find(uri);
	if (it == documents.end()) {
		Logger::warn("Attempt to update non-existent document: " + uri);
		return DocumentError::DOCUMENT_NOT_FOUND;
	}

	// تحديث المستند
	try {
		size_t oldSize = it->second.length();
		documents[uri] = text;
		Logger::debug("Document updated: " + uri + " (" + std::to_string(oldSize) +
			" -> " + std::to_string(text.length()) + " chars)");
		return DocumentError::SUCCESS;
	}
	catch (const std::exception& e) {
		Logger::error("Failed to update document " + uri + ": " + std::string(e.what()));
		return DocumentError::OPERATION_FAILED;
	}
}

// إغلاق مستند  
DocumentError DocumentManager::closeDocument(const std::string& uri) {
	// التحقق من صحة URI
	if (!isValidURI(uri)) {
		Logger::warn("Attempt to close document with invalid URI: " + uri);
		return DocumentError::INVALID_URI;
	}

	// التحقق من وجود المستند
	auto it = documents.find(uri);
	if (it == documents.end()) {
		Logger::warn("Attempt to close non-existent document: " + uri);
		return DocumentError::DOCUMENT_NOT_FOUND;
	}

	// إزالة المستند
	try {
		documents.erase(it);
		Logger::info("Document closed: " + uri);
		return DocumentError::SUCCESS;
	}
	catch (const std::exception& e) {
		Logger::error("Failed to close document " + uri + ": " + std::string(e.what()));
		return DocumentError::OPERATION_FAILED;
	}
}

// الحصول على نص المستند
std::string DocumentManager::getDocumentText(const std::string& uri) const {
	auto it = documents.find(uri);
	if (it != documents.end()) {
		return it->second;
	}

	Logger::debug("Requested text for non-existent document: " + uri);
	return "";
}

// التحقق من وجود المستند
bool DocumentManager::hasDocument(const std::string& uri) const {
	return documents.find(uri) != documents.end();
}

// عدد المستندات المفتوحة
size_t DocumentManager::getDocumentCount() const {
	return documents.size();
}

// تحويل رمز الخطأ إلى رسالة نصية
std::string DocumentManager::errorToString(DocumentError error) {
	switch (error) {
	case DocumentError::SUCCESS:
		return "Operation completed successfully";
	case DocumentError::INVALID_URI:
		return "Invalid or malformed URI";
	case DocumentError::DOCUMENT_NOT_FOUND:
		return "Document not found";
	case DocumentError::DOCUMENT_ALREADY_EXISTS:
		return "Document already exists";
	case DocumentError::INVALID_CONTENT:
		return "Invalid document content";
	case DocumentError::OPERATION_FAILED:
		return "Document operation failed";
	default:
		return "Unknown error";
	}
}

// التحقق من صحة URI
bool DocumentManager::isValidURI(const std::string& uri) const {
	// التحقق من أن URI ليس فارغاً
	if (uri.empty()) {
		return false;
	}

	// التحقق من أن URI لا يحتوي على أحرف غير صالحة
	if (uri.find('\0') != std::string::npos) {
		return false;
	}

	// التحقق من طول URI المعقول
	if (uri.length() > 1000) { // حد أقصى معقول
		return false;
	}

	// فحص أساسي لصيغة URI (يجب أن يحتوي على مسار أو بروتوكول)
	// معظم URIs في LSP تبدأ بـ file:// أو تحتوي على /
	if (uri.find("://") == std::string::npos && uri.find('/') == std::string::npos &&
		uri.find('\\') == std::string::npos) {
		return false;
	}

	return true;
}
