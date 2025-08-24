#pragma once

#include <iostream>
#include <string>

// مستويات التسجيل
enum class LogLevel {
	DEBUG = 0,  // تصحيح
	INFO = 1,   // معلومات
	WARN = 2,   // تحذير
	ERROR = 3   // خطأ
};

// فئة التسجيل - للتعامل مع رسائل السجل
class Logger {
public:
	// تعيين مستوى التسجيل الحالي
	static void setLevel(LogLevel level) {
		currentLevel = level;
	}

	// تسجيل رسالة تصحيح
	static void debug(const std::string& message) {
		log(LogLevel::DEBUG, message);
	}

	// تسجيل رسالة معلومات
	static void info(const std::string& message) {
		log(LogLevel::INFO, message);
	}

	// تسجيل رسالة تحذير
	static void warn(const std::string& message) {
		log(LogLevel::WARN, message);
	}

	// تسجيل رسالة خطأ
	static void error(const std::string& message) {
		log(LogLevel::ERROR, message);
	}

private:
	static LogLevel currentLevel;  // المستوى الحالي للتسجيل

	// دالة التسجيل الداخلية
	static void log(LogLevel level, const std::string& message);

	// تحويل مستوى التسجيل إلى نص
	static std::string levelToString(LogLevel level) {
		switch (level) {
		case LogLevel::DEBUG: return "DEBUG";
		case LogLevel::INFO: return "INFO";
		case LogLevel::WARN: return "WARN";
		case LogLevel::ERROR: return "ERROR";
		default: return "UNKNOWN";
		}
	}
};
