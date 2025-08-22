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

json Completion::getSuggestions(const std::string& uri, int line, int character) {
	// Define enhanced completion item structure
	struct CompletionItem {
		std::string label{};
		int kind{};  // LSP CompletionItemKind
		std::string detail{};
		std::string documentation{};
	};

	// Build static suggestions (sorted alphabetically)
	static const std::vector<CompletionItem> suggestions = [] {
		std::vector<CompletionItem> items{};

		// Alif keywords (Kind: 14 - Keyword)
		const std::vector<std::string> keywords = {
			"و", "ك", "توقف", "صنف", "استمر",
			"دالة", "احذف", "اواذا", "والا", "خلل", "نهاية", "لاجل", "من",
			"عام", "اذا", "استورد", "في", "هل", "نطاق", "ليس",
			"او", "مرر", "ارجع", "حاول", "بينما", "عند", "ولد"
		};
		for (const auto& kw : keywords) {
			items.push_back({ kw, 14, "محجوزة", "كلمة مفتاحية محجوزة" });
		}

		// Constants (Kind: 21 - Constant)
		const std::vector<std::string> constants = { "خطا", "عدم", "صح" };
		for (const auto& c : constants) {
			items.push_back({ c, 21, "ثابت", "قيمة ثابتة ضمنية" });
		}

		// Built-in functions (Kind: 3 - Function)
		const std::vector<std::string> functions = {
			"مطلق", "اطبع", "اي", "منطق", "فهرس", "عشري",
			"ادخل", "صحيح", "طول", "مصفوفة", "اقصى", "ادنى", "افتح", "مدى",
			"نص", "اصل", "مترابطة", "نوع"
		};
		for (const auto& fn : functions) {
			items.push_back({ fn, 3, "دالة ضمنية", "دالة من مكتبة ألف الضمنية" });
		}

		// Built-in types (Kind: 7 - Class)
		const std::vector<std::string> types = {
			"منطق", "فهرس", "عشري",
			"صحيح", "مصفوفة", "كائن", "نص", "مترابطة", "نوع"
		};
		for (const auto& t : types) {
			items.push_back({ t, 7, "نوع", "نوع بيانات اساسي" });
		}

		// Sort alphabetically
		std::sort(items.begin(), items.end(),
			[](const CompletionItem& a, const CompletionItem& b) {
				return a.label < b.label;
			});

		return items;
		}();

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
