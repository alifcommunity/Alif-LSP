#pragma once
#include <vector>
#include <string>

// Context types for Alif code
enum class ContextType {
    GLOBAL_SCOPE,           // Top-level, outside any function/class
    FUNCTION_SCOPE,         // Inside a function body
    CONTROL_BLOCK,          // Inside if/while/for blocks
    FUNCTION_PARAMETERS,    // Inside function parameter list
    FUNCTION_CALL          // Inside function call arguments
};

// Information about the current context at cursor position
struct CodeContext {
    ContextType type;
    std::string functionName;      // Name of current function (if in function scope)
    std::vector<std::string> localVariables;  // Variables in current scope
    std::vector<std::string> parameters;      // Function parameters (if in function)
    int scopeDepth;               // How many nested blocks deep
    bool canReturn;               // Whether 'ارجع' keyword is valid here
};

// Context analyzer for Alif code
class ContextAnalyzer {
public:
    // Analyze context at specific cursor position
    CodeContext analyzeContext(const std::string& documentText, int line, int character);

private:
    // Helper methods for parsing
    std::vector<std::string> splitIntoLines(const std::string& text);
    bool isKeyword(const std::string& word, const std::vector<std::string>& keywords);
    bool isInsideFunction(const std::vector<std::string>& lines, int currentLine);
    std::string extractFunctionName(const std::string& line);
    int calculateScopeDepth(const std::vector<std::string>& lines, int currentLine);
    
    // Alif language keywords
    const std::vector<std::string> FUNCTION_KEYWORDS = {"دالة"};
    const std::vector<std::string> CONTROL_KEYWORDS = {"اذا", "والا", "لاجل", "بينما", "حاول"};
    const std::vector<std::string> BLOCK_STARTERS = {"{", "اذا", "لاجل", "بينما", "دالة", "حاول"};
    const std::vector<std::string> BLOCK_ENDERS = {"}", "نهاية"};
};
