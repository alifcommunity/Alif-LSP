#pragma once
#include <string>
#include <unordered_map>

// أنواع الأخطاء المحتملة في إدارة المستندات
enum class DocumentError {
	SUCCESS = 0,
	INVALID_URI,
	DOCUMENT_NOT_FOUND,
	DOCUMENT_ALREADY_EXISTS,
	INVALID_CONTENT,
	OPERATION_FAILED
};

class DocumentManager {
public:
	// إدارة المستندات مع معالجة الأخطاء
	DocumentError openDocument(const std::string& uri, const std::string& text);
	DocumentError updateDocument(const std::string& uri, const std::string& text);
	DocumentError closeDocument(const std::string& uri);
	
	// الحصول على نص المستند مع التحقق من الوجود
	std::string getDocumentText(const std::string& uri) const;
	bool hasDocument(const std::string& uri) const;
	size_t getDocumentCount() const;
	
	// تحويل رمز الخطأ إلى رسالة نصية
	static std::string errorToString(DocumentError error);

private:
	std::unordered_map<std::string, std::string> documents;
	
	// التحقق من صحة URI
	bool isValidURI(const std::string& uri) const;
};