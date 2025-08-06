#pragma once
#include <string>
#include <unordered_map>

class DocumentManager {
public:
	void openDocument(const std::string& uri, const std::string& text);
	void updateDocument(const std::string& uri, const std::string& text);
	void closeDocument(const std::string& uri);
	std::string getDocumentText(const std::string& uri) const;

private:
	std::unordered_map<std::string, std::string> documents;
};