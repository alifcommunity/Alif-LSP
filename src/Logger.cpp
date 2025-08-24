#include "Logger.h"




// تهيئة المتغير الثابت - المستوى الافتراضي للتسجيل
LogLevel Logger::currentLevel = LogLevel::INFO;



void Logger::log(LogLevel level, const std::string& message) {
	if (level >= currentLevel) {
		std::cerr << "[Alif-LSP] " << levelToString(level) << ": " << message << std::endl;
	}
}
