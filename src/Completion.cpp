#include "Completion.h"
#include "DocManager.h"

extern DocumentManager docManager;

json Completion::getSuggestions(const std::string& uri, int line, int character) {
	// Example: Return static keywords
	json items = json::array();
	items.push_back({
		{"label", "دالة"},
		{"kind", 3},
		{"detail", "كلمة مفتاحية في لغة ألف"}
		});
	items.push_back({
		{"label", "لاجل"},
		{"kind", -1},
		{"detail", "كلمة مفتاحية في لغة ألف"}
		});
	return { {"isIncomplete", false}, {"items", items} };
}