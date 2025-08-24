# Alif LSP Server Makefile for Linux
# المتغيرات الأساسية

# مترجم C++
CXX = g++

# مسارات المشروع
SRC_DIR = ../src
INCLUDE_DIR = $(SRC_DIR)/include
THIRD_PARTY_DIR = $(SRC_DIR)/third-party

# اسم الملف التنفيذي النهائي
TARGET = alif-lsp

# مجلد الإخراج
BUILD_DIR = build

# إعدادات المترجم
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -I$(THIRD_PARTY_DIR)

# إعدادات لوضع التصحيح
DEBUG_FLAGS = -g -O0 -DDEBUG

# إعدادات لوضع الإنتاج
RELEASE_FLAGS = -O2 -DNDEBUG

# ملفات المصدر
SOURCES = $(SRC_DIR)/AlifLSP.cpp \
          $(SRC_DIR)/Server.cpp \
          $(SRC_DIR)/DocManager.cpp \
          $(SRC_DIR)/Completion.cpp \
          $(SRC_DIR)/Logger.cpp

# ملفات الكائنات (objects)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# الهدف الافتراضي
.PHONY: all debug release clean

all: release

# بناء وضع التصحيح
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# بناء وضع الإنتاج
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# ربط الملف التنفيذي
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET)
	@echo "Build completed successfully!"

# قاعدة بناء ملفات الكائنات
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# إنشاء مجلد البناء
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# تنظيف ملفات البناء
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Clean completed!"

# إظهار معلومات المساعدة
help:
	@echo "Alif LSP Server Build System"
	@echo "Available targets:"
	@echo "  all      - Build release version (default)"
	@echo "  debug    - Build debug version with symbols"
	@echo "  release  - Build optimized release version"
	@echo "  clean    - Remove all build files"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make         # Build release version"
	@echo "  make debug   # Build debug version"
	@echo "  make clean   # Clean all files"
