#pragma once
#include <vector>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

// أنواع السياق للإكمال الذكي
enum class CompletionContextType {
	GENERAL,           // عام - أي مكان
	AFTER_FUNCTION,    // بعد كلمة "دالة"
	AFTER_CLASS,       // بعد كلمة "صنف"
	AFTER_IF,          // بعد كلمة "اذا"
	IN_FUNCTION_BODY,  // داخل جسم دالة
	IN_EXPRESSION,     // داخل تعبير
	AFTER_DOT,         // بعد نقطة (للخصائص)
	START_OF_LINE      // بداية السطر
};

struct CompletionContext {
	CompletionContextType type = CompletionContextType::GENERAL;
	std::string previousWord;
	bool inFunctionScope = false;
	bool inClassScope = false;
};

class Completion {
public:
	json getSuggestions(const std::string& uri, int line, int character);

private:
	// دوال مساعدة لتحليل السياق
	std::string getLineText(const std::string& documentText, int line);
	std::string getWordPrefix(const std::string& lineText, int character);
	CompletionContext analyzeContext(const std::string& documentText, int line, int character);
	
	// التصفية والترتيب
	std::vector<json> filterByPrefix(const std::vector<json>& items, const std::string& prefix);
	std::vector<json> filterByContext(const std::vector<json>& items, const CompletionContext& context);
};
