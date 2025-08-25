# Alif LSP Server Makefile for Linux
# المتغيرات الأساسية

# مترجم C++
CXX = g++

# مسارات المشروع
SRC_DIR = ../src
INCLUDE_DIR = $(SRC_DIR)/include
THIRD_PARTY_DIR = $(SRC_DIR)/third-party

# اسم الملف التنفيذي النهائي (سيتم إضافة لاحقة المعمارية)
TARGET_BASE = alif-lsp
TARGET = $(TARGET_BASE)$(TARGET_SUFFIX)

# مجلد الإخراج
BUILD_DIR = build

# إعدادات المترجم
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -I$(THIRD_PARTY_DIR)

# معمارية الهدف (يمكن تخصيصها)
ARCH ?= native

# إعدادات المعمارية
ifeq ($(ARCH),x86_64)
    ARCH_FLAGS = -m64
    TARGET_SUFFIX = _x64
else ifeq ($(ARCH),i386)
    ARCH_FLAGS = -m32
    TARGET_SUFFIX = _x32
else ifeq ($(ARCH),aarch64)
    ARCH_FLAGS = -march=armv8-a
    TARGET_SUFFIX = _arm64
else ifeq ($(ARCH),armv7)
    ARCH_FLAGS = -march=armv7-a -mfpu=neon
    TARGET_SUFFIX = _armv7
else ifeq ($(ARCH),native)
    ARCH_FLAGS = -march=native
    TARGET_SUFFIX = 
else
    $(error Unsupported architecture: $(ARCH))
endif

# إضافة راجات المعمارية للمترجم
CXXFLAGS += $(ARCH_FLAGS)

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
	@echo "Supported architectures (ARCH variable):"
	@echo "  native   - Native architecture (default, optimized for current CPU)"
	@echo "  x86_64   - 64-bit x86 architecture"
	@echo "  i386     - 32-bit x86 architecture"
	@echo "  aarch64  - 64-bit ARM architecture (ARM64)"
	@echo "  armv7    - 32-bit ARMv7 architecture"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build release version (native)"
	@echo "  make debug              # Build debug version (native)"
	@echo "  make ARCH=x86_64        # Build for 64-bit x86"
	@echo "  make ARCH=aarch64       # Build for ARM64"
	@echo "  make ARCH=i386 debug    # Build 32-bit debug version"
	@echo "  make clean              # Clean all files"
	@echo ""
	@echo "Output binary will be named: $(TARGET_BASE)[_suffix]"
