#include "Completion.h"
#include "DocManager.h"
#include "Logger.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

extern DocumentManager docManager;


json Completion::getSuggestions(const std::string& uri, int line, int character) {
	// الحصول على محتوى المستند لتحليل السياق
	std::string documentText = docManager.getDocumentText(uri);
	
	// استخراج السطر الحالي والبادئة
	std::string currentLine = getLineText(documentText, line);
	std::string prefix = getWordPrefix(currentLine, character);
	
	// تحليل السياق حول موضع المؤشر
	CompletionContext context = analyzeContext(documentText, line, character);
	// تعريف بنية عنصر الإكمال المحسنة
	struct CompletionItem {
		std::string label{};
		int kind{};  // نوع عنصر الإكمال في LSP
		std::string detail{};
		std::string documentation{};
	};

	// بناء الاقتراحات الثابتة (مرتبة أبجديا)
	static const std::vector<CompletionItem> suggestions = [] {
		std::vector<CompletionItem> items{};

		// كلمات ألف المحجوزة (النوع: 14 - كلمة محجوزة)
		const std::vector<std::string> keywords = {
			"و", "ك", "توقف", "صنف", "استمر",
			"دالة", "احذف", "اواذا", "والا", "خلل", "نهاية", "لاجل", "من",
			"عام", "اذا", "استورد", "في", "هل", "نطاق", "ليس",
			"او", "مرر", "ارجع", "حاول", "بينما", "عند", "ولد"
		};
		for (const auto& kw : keywords) {
			items.push_back({ kw, 14, "محجوزة", "كلمة مفتاحية محجوزة" });
		}

		// الثوابت (النوع: 21 - ثابت)
		const std::vector<std::string> constants = { "خطا", "عدم", "صح" };
		for (const auto& c : constants) {
			items.push_back({ c, 21, "ثابت", "قيمة ثابتة ضمنية" });
		}

		// الدوال المدمجة (النوع: 3 - دالة)
		const std::vector<std::string> functions = {
			"مطلق", "اطبع", "اي", "منطق", "فهرس", "عشري",
			"ادخل", "صحيح", "طول", "مصفوفة", "اقصى", "ادنى", "افتح", "مدى",
			"نص", "اصل", "مترابطة", "نوع"
		};
		for (const auto& fn : functions) {
			items.push_back({ fn, 3, "دالة ضمنية", "دالة من مكتبة ألف الضمنية" });
		}

		// الأنواع المدمجة (النوع: 7 - صنف)
		const std::vector<std::string> types = {
			"منطق", "فهرس", "عشري",
			"صحيح", "مصفوفة", "كائن", "نص", "مترابطة", "نوع"
		};
		for (const auto& t : types) {
			items.push_back({ t, 7, "نوع", "نوع بيانات اساسي" });
		}

		// ترتيب أبجدي
		std::sort(items.begin(), items.end(),
			[](const CompletionItem& a, const CompletionItem& b) {
				return a.label < b.label;
			});

		return items;
		}();

	// تحويل إلى JSON مع معالجة مناسبة للمقاطع
	std::vector<json> items;
	for (const auto& item : suggestions) {
		json j = {
			{"label", item.label},
			{"kind", item.kind},
			{"detail", item.detail},
			{"documentation", item.documentation}
		};
		items.push_back(j);
	}

	// تطبيق التصفية حسب السياق
	std::vector<json> contextFilteredItems = filterByContext(items, context);
	
	// تطبيق التصفية حسب البادئة
	std::vector<json> prefixFilteredItems = filterByPrefix(contextFilteredItems, prefix);
	
	// تسجيل معلومات الإكمال
	Logger::debug("Completion: prefix='" + prefix + "', context=" + std::to_string(static_cast<int>(context.type)) + 
		", total=" + std::to_string(prefixFilteredItems.size()) + " items");
	
	// تحويل النتيجة النهائية إلى JSON array
	json finalItems = json::array();
	for (const auto& item : prefixFilteredItems) {
		finalItems.push_back(item);
	}
	
	return { {"isIncomplete", false}, {"items", finalItems} };
}

// تنفيذ الدوال المساعدة
std::string Completion::getLineText(const std::string& documentText, int line) {
	std::istringstream stream(documentText);
	std::string currentLine;
	int currentLineNumber = 0;
	
	while (std::getline(stream, currentLine) && currentLineNumber < line) {
		currentLineNumber++;
	}
	
	return (currentLineNumber == line) ? currentLine : "";
}

std::string Completion::getWordPrefix(const std::string& lineText, int character) {
	if (character > static_cast<int>(lineText.length())) {
		character = static_cast<int>(lineText.length());
	}
	
	// العثور على بداية الكلمة الحالية (للخلف من المؤشر)
	int start = character;
	while (start > 0 && (std::isalnum(lineText[start - 1]) || lineText[start - 1] == '_')) {
		start--;
	}
	
	// دعم الأحرف العربية (UTF-8)
	while (start > 0) {
		unsigned char c = lineText[start - 1];
		if ((c >= 0xD8 && c <= 0xDF) || (c >= 0x80 && c <= 0xBF)) {  // نطاق الأحرف العربية
			start--;
		} else {
			break;
		}
	}
	
	return lineText.substr(start, character - start);
}

CompletionContext Completion::analyzeContext(const std::string& documentText, int line, int character) {
	CompletionContext context;
	
	// الحصول على السطر الحالي للتحليل
	std::string currentLine = getLineText(documentText, line);
	
	// إزالة المسافات والعثور على الكلمة السابقة
	std::string trimmedLine = currentLine.substr(0, character);
	std::istringstream words(trimmedLine);
	std::string word;
	std::string previousWord;
	
	while (words >> word) {
		previousWord = word;
	}
	
	context.previousWord = previousWord;
	
	// تحديد نوع السياق بناءً على الكلمة السابقة
	if (previousWord == "دالة") {
		context.type = CompletionContextType::AFTER_FUNCTION;
	} else if (previousWord == "صنف") {
		context.type = CompletionContextType::AFTER_CLASS;
	} else if (previousWord == "اذا" || previousWord == "اواذا" || previousWord == "والا") {
		context.type = CompletionContextType::AFTER_IF;
	} else if (trimmedLine.find('.') != std::string::npos && 
		       trimmedLine.rfind('.') == trimmedLine.length() - 1) {
		context.type = CompletionContextType::AFTER_DOT;
	} else if (trimmedLine.empty() || trimmedLine.find_first_not_of(" \t") == std::string::npos) {
		context.type = CompletionContextType::START_OF_LINE;
	} else if (trimmedLine.find('(') != std::string::npos || 
		       trimmedLine.find('+') != std::string::npos ||
		       trimmedLine.find('=') != std::string::npos) {
		context.type = CompletionContextType::IN_EXPRESSION;
	} else {
		context.type = CompletionContextType::GENERAL;
	}
	
	// فحص ما إذا كنا داخل نطاق دالة أو صنف (تحليل مبسط)
	context.inFunctionScope = documentText.find("دالة") != std::string::npos;
	context.inClassScope = documentText.find("صنف") != std::string::npos;
	
	return context;
}

std::vector<json> Completion::filterByPrefix(const std::vector<json>& items, const std::string& prefix) {
	if (prefix.empty()) {
		return items;
	}
	
	std::vector<json> filtered;
	try {
		for (const auto& item : items) {
			if (item.contains("label") && item["label"].is_string()) {
				std::string label = item["label"];
				if (!label.empty() && label.find(prefix) == 0) {  // يبدأ بالبادئة
					filtered.push_back(item);
				}
			}
		}
	}
	catch (const std::exception& e) {
		Logger::error("Error in filterByPrefix: " + std::string(e.what()));
		return items;  // Return original items on error
	}
	
	return filtered;
}

std::vector<json> Completion::filterByContext(const std::vector<json>& items, const CompletionContext& context) {
	std::vector<json> filtered;
	
	try {
		for (const auto& item : items) {
			if (!item.contains("kind") || !item.contains("label") || 
				!item["kind"].is_number() || !item["label"].is_string()) {
				continue;  // Skip malformed items
			}
			
			int kind = item["kind"];
			std::string label = item["label"];
			bool includeItem = true;
		
		switch (context.type) {
		case CompletionContextType::AFTER_FUNCTION:
			// بعد "دالة"، تفضيل المعرفات وليس الكلمات المحجوزة
			includeItem = (kind != 14);  // ليس كلمات محجوزة
			break;
			
		case CompletionContextType::AFTER_CLASS:
			// بعد "صنف"، تفضيل المعرفات
			includeItem = (kind != 14);  // ليس كلمات محجوزة
			break;
			
		case CompletionContextType::AFTER_IF:
			// بعد "اذا"، تفضيل التعبيرات والقيم
			includeItem = (kind == 3 || kind == 21 || kind == 7);  // دوال، ثوابت، أنواع
			break;
			
		case CompletionContextType::IN_EXPRESSION:
			// داخل التعبيرات، تفضيل الدوال والقيم
			includeItem = (kind == 3 || kind == 21);  // دوال وثوابت
			break;
			
		case CompletionContextType::START_OF_LINE:
			// في بداية السطر، تفضيل كلمات التحكم في التدفق
			includeItem = (kind == 14 || kind == 3);  // كلمات محجوزة ودوال
			break;
			
		case CompletionContextType::AFTER_DOT:
			// بعد النقطة، تفضيل الطرق/الخصائص (حالياً نفس الدوال)
			includeItem = (kind == 3);  // دوال
			break;
			
		default:
			// السياق العام - تضمين كل شيء
			includeItem = true;
			break;
		}
		
			if (includeItem) {
				filtered.push_back(item);
			}
		}
	}
	catch (const std::exception& e) {
		Logger::error("Error in filterByContext: " + std::string(e.what()));
		return items;  // Return original items on error
	}
	
	return filtered;
}
