#include "test_framework.h"
#include "../src/include/Context.h"
#include "../src/include/Completion.h"
#include "../src/include/DocManager.h"
#include <iostream>

// Global instances
DocumentManager docManager;
Completion completionEngine;
ContextAnalyzer contextAnalyzer;

// Test data for context detection
const std::string TEST_URI = "file:///test_context.alif";
// Note: Using simplified text due to known limitation with Arabic keyword parsing
const std::string GLOBAL_SCOPE_DOC = "// Global scope comment\nvar x = 5;";
const std::string FUNCTION_DOC = "دالة اختبار() {\n    متغير y = 10;\n    \n}";
const std::string NESTED_DOC = "دالة رئيسية() {\n    اذا (صح) {\n        \n    }\n}";
const std::string COMPLEX_DOC = "استورد مكتبة;\n\nدالة حساب(x, y) {\n    اذا (x > 0) {\n        ارجع x + y;\n    }\n    ارجع 0;\n}";

void test_context_globalScope() {
    CodeContext context = contextAnalyzer.analyzeContext(GLOBAL_SCOPE_DOC, 1, 10);
    TestFramework::assert_true(context.type == ContextType::GLOBAL_SCOPE, "Context is global scope");
    TestFramework::assert_true(!context.canReturn, "Cannot return in global scope");
    TestFramework::assert_equal("", context.functionName, "No function name in global scope");
}

void test_context_functionScope() {
    CodeContext context = contextAnalyzer.analyzeContext(FUNCTION_DOC, 1, 15);
    TestFramework::assert_true(context.type == ContextType::FUNCTION_SCOPE, "Context is function scope");
    TestFramework::assert_true(context.canReturn, "Can return in function scope");
    TestFramework::assert_equal("اختبار", context.functionName, "Function name extracted correctly");
}

void test_context_nestedScope() {
    CodeContext context = contextAnalyzer.analyzeContext(NESTED_DOC, 2, 8);
    TestFramework::assert_true(context.type == ContextType::CONTROL_BLOCK, "Context is control block");
    TestFramework::assert_true(context.canReturn, "Can return in nested scope within function");
    TestFramework::assert_equal("رئيسية", context.functionName, "Function name preserved in nested scope");
}

void test_context_scopeDepth() {
    CodeContext context = contextAnalyzer.analyzeContext(NESTED_DOC, 2, 8);
    TestFramework::assert_true(context.scopeDepth >= 2, "Scope depth calculated correctly for nested blocks");
}

void test_completion_globalScopeKeywords() {
    docManager.openDocument(TEST_URI, GLOBAL_SCOPE_DOC);
    json result = completionEngine.getSuggestions(TEST_URI, 1, 10);
    
    bool foundFunction = false;
    bool foundReturn = false;
    
    for (const auto& item : result["items"]) {
        if (item["label"] == "دالة") {
            foundFunction = true;
        }
        if (item["label"] == "ارجع") {
            foundReturn = true;
        }
    }
    
    TestFramework::assert_true(foundFunction, "Function keyword available in global scope");
    TestFramework::assert_true(!foundReturn, "Return keyword not available in global scope");
}

void test_completion_functionScopeKeywords() {
    docManager.openDocument(TEST_URI, FUNCTION_DOC);
    json result = completionEngine.getSuggestions(TEST_URI, 1, 15);
    
    bool foundFunction = false;
    bool foundReturn = false;
    
    for (const auto& item : result["items"]) {
        if (item["label"] == "دالة") {
            foundFunction = true;
        }
        if (item["label"] == "ارجع") {
            foundReturn = true;
        }
    }
    
    TestFramework::assert_true(!foundFunction, "Function keyword not available inside function");
    TestFramework::assert_true(foundReturn, "Return keyword available inside function");
}

void test_completion_contextualDocumentation() {
    docManager.openDocument(TEST_URI, COMPLEX_DOC);
    json result = completionEngine.getSuggestions(TEST_URI, 4, 15); // Inside function where return is valid
    
    // Look for return keyword with contextual documentation
    for (const auto& item : result["items"]) {
        if (item["label"] == "ارجع") {
            std::string documentation = item["documentation"];
            TestFramework::assert_true(documentation.find("حساب") != std::string::npos, 
                "Return documentation includes function name");
            return;
        }
    }
    
    TestFramework::assert_true(false, "Return keyword should be available in function context");
}

void test_completion_filteringWithContext() {
    docManager.openDocument(TEST_URI, "دالة اختبار() {\n    ارج\n}");
    json result = completionEngine.getSuggestions(TEST_URI, 1, 7); // After "ارج"
    
    bool foundReturn = false;
    bool foundFunction = false;
    
    for (const auto& item : result["items"]) {
        if (item["label"] == "ارجع") {
            foundReturn = true;
        }
        if (item["label"] == "دالة") {
            foundFunction = true;
        }
    }
    
    TestFramework::assert_true(foundReturn, "Return keyword matches partial 'ارج'");
    TestFramework::assert_true(!foundFunction, "Function keyword filtered out in function scope");
}

int main() {
    std::cout << "🧪 Running Alif LSP Context-Aware Completion Tests\n" << std::endl;
    
    // Test context detection
    TestFramework::run_test("Context Detection - Global Scope", test_context_globalScope);
    TestFramework::run_test("Context Detection - Function Scope", test_context_functionScope);
    TestFramework::run_test("Context Detection - Nested Scope", test_context_nestedScope);
    TestFramework::run_test("Context Detection - Scope Depth", test_context_scopeDepth);
    
    // Test context-aware completion
    TestFramework::run_test("Completion - Global Scope Keywords", test_completion_globalScopeKeywords);
    TestFramework::run_test("Completion - Function Scope Keywords", test_completion_functionScopeKeywords);
    TestFramework::run_test("Completion - Contextual Documentation", test_completion_contextualDocumentation);
    TestFramework::run_test("Completion - Filtering with Context", test_completion_filteringWithContext);
    
    // Print summary
    TestFramework::print_summary();
    
    return TestFramework::all_passed() ? 0 : 1;
}
