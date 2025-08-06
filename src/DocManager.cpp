#include "DocManager.h"

void DocumentManager::openDocument(const std::string& uri, const std::string& text) {
	documents[uri] = text;
}

void DocumentManager::updateDocument(const std::string& uri, const std::string& text) {
	if (documents.find(uri) != documents.end()) {
		documents[uri] = text;
	}
}

void DocumentManager::closeDocument(const std::string& uri) {
	documents.erase(uri);
}

std::string DocumentManager::getDocumentText(const std::string& uri) const {
	auto it = documents.find(uri);
	return (it != documents.end()) ? it->second : "";
}