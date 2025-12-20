# ------------------------------
# IoTropolis Makefile (Qt5/Qt6, automatic MOC, recursive)
# ------------------------------

CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC
LDFLAGS =

# ------------------------------
# Detect Qt version via pkg-config
# ------------------------------
QT_MODULES := Qt6Core Qt6Network Qt6Widgets
QT_PKG := $(shell pkg-config --exists $(QT_MODULES) && echo 6 || echo 5)

ifeq ($(QT_PKG),6)
    QT_MODULES := Qt6Core Qt6Network Qt6Widgets
    QT_CFLAGS := $(shell pkg-config --cflags $(QT_MODULES))
    QT_LDFLAGS := $(shell pkg-config --libs $(QT_MODULES))
    MOC := $(shell pkg-config --variable=libexecdir Qt6Core 2>/dev/null)/moc

else
    QT_MODULES := Qt5Core Qt5Network Qt5Widgets
    QT_CFLAGS := $(shell pkg-config --cflags $(QT_MODULES))
    QT_LDFLAGS := $(shell pkg-config --libs $(QT_MODULES))
    MOC := $(shell pkg-config --variable=host_bins Qt5Core 2>/dev/null)/moc
endif

CXXFLAGS += $(QT_CFLAGS)
LDFLAGS += $(QT_LDFLAGS)

INCLUDES = -I./include
BUILD_DIR = build
TARGET = iotropolis

# ------------------------------
# Find all source files recursively
# ------------------------------
SRCS := $(shell find src -name "*.cpp")
# Generate object files under build/ mirroring src/ structure
OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# ------------------------------
# Find all headers with Q_OBJECT recursively
# ------------------------------
MOC_HEADERS := $(shell grep -rl "Q_OBJECT" include/)

# MOC source files
MOC_SRCS := $(patsubst include/%, $(BUILD_DIR)/moc/%, $(MOC_HEADERS:.h=.cpp))
MOC_OBJS := $(MOC_SRCS:.cpp=.o)

# ------------------------------
# Default target
# ------------------------------
all: $(TARGET)

# ------------------------------
# Link executable
# ------------------------------
$(TARGET): $(OBJS) $(MOC_OBJS)
	$(CXX) -o $@ $(OBJS) $(MOC_OBJS) $(LDFLAGS)

# ------------------------------
# Compile normal source files
# ------------------------------
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------
# Generate MOC sources
# ------------------------------
$(BUILD_DIR)/moc/%.cpp: include/%.h
	@mkdir -p $(dir $@)
	$(MOC) -o $@ $<

# ------------------------------
# Compile MOC-generated sources
# ------------------------------
$(BUILD_DIR)/moc/%.o: $(BUILD_DIR)/moc/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------
# Clean
# ------------------------------
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
