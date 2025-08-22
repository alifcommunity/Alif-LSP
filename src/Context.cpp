#include "Context.h"
#include <algorithm>
#include <cctype>
#include <sstream>

std::vector<std::string> ContextAnalyzer::splitIntoLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string currentLine;
    
    for (char c : text) {
        if (c == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    
    // Add the last line if it doesn't end with newline
    if (!currentLine.empty() || (!text.empty() && text.back() != '\n')) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

bool ContextAnalyzer::isKeyword(const std::string& word, const std::vector<std::string>& keywords) {
    return std::find(keywords.begin(), keywords.end(), word) != keywords.end();
}

std::string ContextAnalyzer::extractFunctionName(const std::string& line) {
	// Look for pattern: "دالة functionName"
	size_t functionPos = line.find("دالة");
	if (functionPos == std::string::npos) {
		return "";
	}
	
	// Find the start of function name (after "دالة" and whitespace)
	size_t nameStart = functionPos + 8; // "دالة" is 8 bytes in UTF-8 (4 chars * 2 bytes each)
	while (nameStart < line.length() && std::isspace(static_cast<unsigned char>(line[nameStart]))) {
		nameStart++;
	}
	
	if (nameStart >= line.length()) {
		return "";
	}
	
	// Find the end of function name (before whitespace, '(' or '{')
	size_t nameEnd = nameStart;
	while (nameEnd < line.length()) {
		unsigned char c = static_cast<unsigned char>(line[nameEnd]);
		if (std::isspace(c) || line[nameEnd] == '(' || line[nameEnd] == '{') {
			break;
		}
		nameEnd++;
	}
	
	if (nameEnd <= nameStart) {
		return "";
	}
	
	return line.substr(nameStart, nameEnd - nameStart);
}

bool ContextAnalyzer::isInsideFunction(const std::vector<std::string>& lines, int currentLine) {
    int braceCount = 0;
    bool foundFunction = false;
    
    // Go backwards from current line to find function start
    for (int i = currentLine; i >= 0; i--) {
        const std::string& line = lines[i];
        
        // Count braces to track scope
        for (char c : line) {
            if (c == '{') {
                braceCount++;
            } else if (c == '}') {
                braceCount--;
                if (braceCount < 0) {
                    // We've exited the function scope
                    return false;
                }
            }
        }
        
        // Check if this line contains a function declaration
        if (line.find("دالة") != std::string::npos && braceCount > 0) {
            foundFunction = true;
            break;
        }
    }
    
    return foundFunction && braceCount > 0;
}

int ContextAnalyzer::calculateScopeDepth(const std::vector<std::string>& lines, int currentLine) {
    int depth = 0;
    
    for (int i = 0; i <= currentLine; i++) {
        const std::string& line = lines[i];
        
        for (char c : line) {
            if (c == '{') {
                depth++;
            } else if (c == '}') {
                depth--;
            }
        }
    }
    
    return std::max(0, depth);
}

CodeContext ContextAnalyzer::analyzeContext(const std::string& documentText, int line, int character) {
    CodeContext context;
    context.type = ContextType::GLOBAL_SCOPE;
    context.functionName = "";
    context.scopeDepth = 0;
    context.canReturn = false;
    
    if (documentText.empty()) {
        return context;
    }
    
    std::vector<std::string> lines = splitIntoLines(documentText);
    
    // Check if line number is valid
    if (line < 0 || line >= static_cast<int>(lines.size())) {
        return context;
    }
    
    // Calculate scope depth
    context.scopeDepth = calculateScopeDepth(lines, line);
    
    // Check if we're inside a function
    if (isInsideFunction(lines, line)) {
        context.type = ContextType::FUNCTION_SCOPE;
        context.canReturn = true;
        
        // Find the function name by going backwards
        for (int i = line; i >= 0; i--) {
            std::string funcName = extractFunctionName(lines[i]);
            if (!funcName.empty()) {
                context.functionName = funcName;
                break;
            }
        }
    }
    
    // Check for control block context
    if (context.scopeDepth > 1 || 
        (context.type == ContextType::FUNCTION_SCOPE && context.scopeDepth > 1)) {
        // Look for control flow keywords on current or recent lines
        for (int i = std::max(0, line - 3); i <= line; i++) {
            if (i < static_cast<int>(lines.size())) {
                const std::string& checkLine = lines[i];
                for (const std::string& keyword : CONTROL_KEYWORDS) {
                    if (checkLine.find(keyword) != std::string::npos) {
                        context.type = ContextType::CONTROL_BLOCK;
                        break;
                    }
                }
            }
        }
    }
    
    return context;
}
