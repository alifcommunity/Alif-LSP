#include "Completion.h"
#include "DocManager.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cctype>

extern DocumentManager docManager;

std::string Completion::getCurrentWord(const std::string& uri, int line, int character) {
	std::string documentText = docManager.getDocumentText(uri);
	if (documentText.empty()) {
		return "";
	}

	// Split document into lines
	std::vector<std::string> lines;
	std::string currentLineText;
	for (char c : documentText) {
		if (c == '\n') {
			lines.push_back(currentLineText);
			currentLineText.clear();
		} else {
			currentLineText += c;
		}
	}
	// Add the last line if it doesn't end with newline
	if (!currentLineText.empty() || documentText.back() != '\n') {
		lines.push_back(currentLineText);
	}

	// Check if line number is valid
	if (line < 0 || line >= static_cast<int>(lines.size())) {
		return "";
	}

	const std::string& targetLine = lines[line];
	
	// For simplicity, treat character position as byte position
	// This works for LSP since editors typically send byte positions
	if (character < 0 || character > static_cast<int>(targetLine.length())) {
		return "";
	}

	// Find word start by going backwards from cursor
	int wordStart = character;
	while (wordStart > 0) {
		char prevChar = targetLine[wordStart - 1];
		// Break on whitespace or common punctuation
		if (std::isspace(static_cast<unsigned char>(prevChar)) || 
			prevChar == '(' || prevChar == ')' || prevChar == '{' || prevChar == '}' ||
			prevChar == '[' || prevChar == ']' || prevChar == ';' || prevChar == ',' ||
			prevChar == '.' || prevChar == ':') {
			break;
		}
		wordStart--;
	}

	// Extract the word from wordStart to character position
	if (wordStart >= character) {
		return "";
	}

	return targetLine.substr(wordStart, character - wordStart);
}

std::vector<std::string> Completion::getContextualKeywords(const CodeContext& context) {
	std::vector<std::string> keywords;
	
	// Always available keywords
	std::vector<std::string> commonKeywords = {
		"و", "ك", "توقف", "صنف", "استمر", "احذف", "اواذا", "والا", 
		"خلل", "نهاية", "لاجل", "من", "اذا", "في", "هل", "نطاق", 
		"ليس", "او", "مرر", "حاول", "بينما", "عند", "ولد"
	};
	
	keywords.insert(keywords.end(), commonKeywords.begin(), commonKeywords.end());
	
	// Context-specific keywords
	switch (context.type) {
		case ContextType::GLOBAL_SCOPE:
			// Add global-only keywords
			keywords.push_back("دالة");  // Function definition only at global scope
			keywords.push_back("استورد"); // Import only at global scope
			keywords.push_back("عام");    // Global modifier
			break;
			
		case ContextType::FUNCTION_SCOPE:
		case ContextType::CONTROL_BLOCK:
			// Add function-only keywords
			if (context.canReturn) {
				keywords.push_back("ارجع"); // Return only inside functions
			}
			break;
			
		case ContextType::FUNCTION_PARAMETERS:
		case ContextType::FUNCTION_CALL:
			// Limited keywords in parameter contexts
			break;
	}
	
	return keywords;
}

json Completion::getSuggestions(const std::string& uri, int line, int character) {
	// Define enhanced completion item structure
	struct CompletionItem {
		std::string label{};
		int kind{};  // LSP CompletionItemKind
		std::string detail{};
		std::string documentation{};
	};

	// Analyze context at cursor position
	std::string documentText = docManager.getDocumentText(uri);
	CodeContext context = contextAnalyzer.analyzeContext(documentText, line, character);
	
	// Get contextual keywords based on current context
	std::vector<std::string> contextualKeywords = getContextualKeywords(context);
	
	// Build dynamic suggestions based on context
	std::vector<CompletionItem> suggestions;
	
	// Add contextual keywords
	for (const auto& kw : contextualKeywords) {
		std::string detail = "محجوزة";
		std::string docs = "كلمة مفتاحية محجوزة";
		
		// Add context-specific documentation
		if (kw == "ارجع" && context.canReturn) {
			detail = "ارجاع";
			docs = "ارجاع قيمة من الدالة: " + context.functionName;
		} else if (kw == "دالة" && context.type == ContextType::GLOBAL_SCOPE) {
			detail = "تعريف دالة";
			docs = "تعريف دالة جديدة في النطاق العام";
		}
		
				suggestions.push_back({ kw, 14, detail, docs });
	}

	// Add constants (always available)
	const std::vector<std::string> constants = { "خطا", "عدم", "صح" };
	for (const auto& c : constants) {
		suggestions.push_back({ c, 21, "ثابت", "قيمة ثابتة ضمنية" });
	}

	// Add built-in functions (always available)
	const std::vector<std::string> functions = {
		"مطلق", "اطبع", "اي", "منطق", "فهرس", "عشري",
		"ادخل", "صحيح", "طول", "مصفوفة", "اقصى", "ادنى", "افتح", "مدى",
		"نص", "اصل", "مترابطة", "نوع"
	};
	for (const auto& fn : functions) {
		suggestions.push_back({ fn, 3, "دالة ضمنية", "دالة من مكتبة ألف الضمنية" });
	}

	// Add built-in types (always available)
	const std::vector<std::string> types = {
		"منطق", "فهرس", "عشري",
		"صحيح", "مصفوفة", "كائن", "نص", "مترابطة", "نوع"
	};
	for (const auto& t : types) {
		suggestions.push_back({ t, 7, "نوع", "نوع بيانات اساسي" });
	}

	// Sort alphabetically
	std::sort(suggestions.begin(), suggestions.end(),
		[](const CompletionItem& a, const CompletionItem& b) {
			return a.label < b.label;
		});

	// Get the current word being typed at cursor position
	std::string currentWord = getCurrentWord(uri, line, character);
	
	// Filter suggestions based on current word
	std::vector<CompletionItem> filteredSuggestions;
	for (const auto& item : suggestions) {
		// If no current word, show all suggestions
		if (currentWord.empty() || 
			// Check if the suggestion starts with the current word
			item.label.substr(0, currentWord.length()) == currentWord) {
			filteredSuggestions.push_back(item);
		}
	}

	// Convert to JSON with proper snippet handling
	json items = json::array();
	for (const auto& item : filteredSuggestions) {
		json j = {
			{"label", item.label},
			{"kind", item.kind},
			{"detail", item.detail},
			{"documentation", item.documentation}
		};
		items.push_back(j);
	}

	return { {"isIncomplete", false}, {"items", items} };
}
