#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>

// Simple test framework for Alif LSP
class TestFramework {
private:
    static int totalTests;
    static int passedTests;
    static std::vector<std::string> failedTests;

public:
    static void assert_equal(const std::string& expected, const std::string& actual, const std::string& testName) {
        totalTests++;
        if (expected == actual) {
            passedTests++;
            std::cout << "✅ " << testName << " - PASSED" << std::endl;
        } else {
            failedTests.push_back(testName);
            std::cout << "❌ " << testName << " - FAILED" << std::endl;
            std::cout << "   Expected: '" << expected << "'" << std::endl;
            std::cout << "   Actual:   '" << actual << "'" << std::endl;
        }
    }

    static void assert_true(bool condition, const std::string& testName) {
        totalTests++;
        if (condition) {
            passedTests++;
            std::cout << "✅ " << testName << " - PASSED" << std::endl;
        } else {
            failedTests.push_back(testName);
            std::cout << "❌ " << testName << " - FAILED" << std::endl;
            std::cout << "   Expected: true, got: false" << std::endl;
        }
    }

    static void run_test(const std::string& testName, std::function<void()> testFunc) {
        std::cout << "\n--- Running: " << testName << " ---" << std::endl;
        try {
            testFunc();
        } catch (const std::exception& e) {
            totalTests++;
            failedTests.push_back(testName);
            std::cout << "❌ " << testName << " - FAILED (Exception: " << e.what() << ")" << std::endl;
        }
    }

    static void print_summary() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "Total Tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
        
        if (!failedTests.empty()) {
            std::cout << "\nFailed Tests:" << std::endl;
            for (const auto& test : failedTests) {
                std::cout << "  - " << test << std::endl;
            }
        }
        
        if (passedTests == totalTests && totalTests > 0) {
            std::cout << "\n🎉 All tests passed!" << std::endl;
        }
        std::cout << std::string(50, '=') << std::endl;
    }

    static bool all_passed() {
        return passedTests == totalTests && totalTests > 0;
    }
};

// Static member definitions
int TestFramework::totalTests = 0;
int TestFramework::passedTests = 0;
std::vector<std::string> TestFramework::failedTests;
