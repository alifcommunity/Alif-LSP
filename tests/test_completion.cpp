#include "test_framework.h"
#include "../src/include/Completion.h"
#include "../src/include/DocManager.h"
#include <iostream>

// Global instances (same as main application)
DocumentManager docManager;
Completion completionEngine;

// Test data
const std::string TEST_URI = "file:///test.alif";
const std::string EMPTY_DOC = "";
const std::string SIMPLE_DOC = "دالة اختبار";
const std::string MULTILINE_DOC = "دالة اختبار()\n{\n    ارجع صح;\n}";
const std::string PARTIAL_WORD_DOC = "دا";

void test_getCurrentWord_emptyDocument() {
    docManager.openDocument(TEST_URI, EMPTY_DOC);
    std::string result = completionEngine.getCurrentWord(TEST_URI, 0, 0);
    TestFramework::assert_equal("", result, "getCurrentWord with empty document");
}

void test_getCurrentWord_beginningOfLine() {
    docManager.openDocument(TEST_URI, SIMPLE_DOC);
    std::string result = completionEngine.getCurrentWord(TEST_URI, 0, 0);
    TestFramework::assert_equal("", result, "getCurrentWord at beginning of line");
}

void test_getCurrentWord_middleOfWord() {
	docManager.openDocument(TEST_URI, SIMPLE_DOC);
	// Position in middle of "دالة" (Arabic chars are 2 bytes each, so pos 4 = after 2nd char)
	std::string result = completionEngine.getCurrentWord(TEST_URI, 0, 4);
	TestFramework::assert_equal("دا", result, "getCurrentWord in middle of word");
}

void test_getCurrentWord_endOfWord() {
	docManager.openDocument(TEST_URI, SIMPLE_DOC);
	// Position at end of "دالة" (Arabic "دالة" is 8 bytes)
	std::string result = completionEngine.getCurrentWord(TEST_URI, 0, 8);
	TestFramework::assert_equal("دالة", result, "getCurrentWord at end of word");
}

void test_getCurrentWord_afterSpace() {
	docManager.openDocument(TEST_URI, SIMPLE_DOC);
	// Position after space, before "اختبار" (space is at byte 8, so position 9)
	std::string result = completionEngine.getCurrentWord(TEST_URI, 0, 9);
	TestFramework::assert_equal("", result, "getCurrentWord after space");
}

void test_getCurrentWord_partialSecondWord() {
	docManager.openDocument(TEST_URI, SIMPLE_DOC);
	// Position in middle of "اختبار" (after 2 chars = 9 + 4 = 13)
	std::string result = completionEngine.getCurrentWord(TEST_URI, 0, 13);
	TestFramework::assert_equal("اخ", result, "getCurrentWord in partial second word");
}

void test_getCurrentWord_multiline() {
	docManager.openDocument(TEST_URI, MULTILINE_DOC);
	// Position at end of "ارجع" on line 2 (4 spaces + 8 bytes = position 12)
	std::string result = completionEngine.getCurrentWord(TEST_URI, 2, 12);
	TestFramework::assert_equal("ارجع", result, "getCurrentWord in multiline document");
}

void test_getSuggestions_emptyDocument() {
    docManager.openDocument(TEST_URI, EMPTY_DOC);
    json result = completionEngine.getSuggestions(TEST_URI, 0, 0);
    
    TestFramework::assert_true(result.contains("items"), "getSuggestions returns items array");
    TestFramework::assert_true(result["items"].is_array(), "items is an array");
    TestFramework::assert_true(result["items"].size() > 10, "empty document returns multiple suggestions");
}

void test_getSuggestions_partialMatch() {
    docManager.openDocument(TEST_URI, PARTIAL_WORD_DOC);
    json result = completionEngine.getSuggestions(TEST_URI, 0, 2);
    
    bool foundDala = false;
    for (const auto& item : result["items"]) {
        if (item["label"] == "دالة") {
            foundDala = true;
            break;
        }
    }
    
    TestFramework::assert_true(foundDala, "partial 'دا' matches 'دالة'");
    TestFramework::assert_true(result["items"].size() < 50, "filtered results are fewer than total");
}

void test_getSuggestions_exactMatch() {
    docManager.openDocument(TEST_URI, "ارجع");
    json result = completionEngine.getSuggestions(TEST_URI, 0, 4);
    
    bool foundArje = false;
    for (const auto& item : result["items"]) {
        if (item["label"] == "ارجع") {
            foundArje = true;
            break;
        }
    }
    
    TestFramework::assert_true(foundArje, "exact match 'ارجع' found in suggestions");
}

void test_getSuggestions_noMatch() {
    docManager.openDocument(TEST_URI, "xyz");
    json result = completionEngine.getSuggestions(TEST_URI, 0, 3);
    
    TestFramework::assert_true(result["items"].size() == 0, "no matches for invalid input");
}

void test_getSuggestions_itemStructure() {
    docManager.openDocument(TEST_URI, EMPTY_DOC);
    json result = completionEngine.getSuggestions(TEST_URI, 0, 0);
    
    if (result["items"].size() > 0) {
        const auto& firstItem = result["items"][0];
        TestFramework::assert_true(firstItem.contains("label"), "suggestion has label");
        TestFramework::assert_true(firstItem.contains("kind"), "suggestion has kind");
        TestFramework::assert_true(firstItem.contains("detail"), "suggestion has detail");
        TestFramework::assert_true(firstItem.contains("documentation"), "suggestion has documentation");
    }
}

int main() {
    std::cout << "🧪 Running Alif LSP Completion Tests\n" << std::endl;
    
    // Test getCurrentWord function
    TestFramework::run_test("getCurrentWord - Empty Document", test_getCurrentWord_emptyDocument);
    TestFramework::run_test("getCurrentWord - Beginning of Line", test_getCurrentWord_beginningOfLine);
    TestFramework::run_test("getCurrentWord - Middle of Word", test_getCurrentWord_middleOfWord);
    TestFramework::run_test("getCurrentWord - End of Word", test_getCurrentWord_endOfWord);
    TestFramework::run_test("getCurrentWord - After Space", test_getCurrentWord_afterSpace);
    TestFramework::run_test("getCurrentWord - Partial Second Word", test_getCurrentWord_partialSecondWord);
    TestFramework::run_test("getCurrentWord - Multiline Document", test_getCurrentWord_multiline);
    
    // Test getSuggestions function
    TestFramework::run_test("getSuggestions - Empty Document", test_getSuggestions_emptyDocument);
    TestFramework::run_test("getSuggestions - Partial Match", test_getSuggestions_partialMatch);
    TestFramework::run_test("getSuggestions - Exact Match", test_getSuggestions_exactMatch);
    TestFramework::run_test("getSuggestions - No Match", test_getSuggestions_noMatch);
    TestFramework::run_test("getSuggestions - Item Structure", test_getSuggestions_itemStructure);
    
    // Print summary
    TestFramework::print_summary();
    
    return TestFramework::all_passed() ? 0 : 1;
}
