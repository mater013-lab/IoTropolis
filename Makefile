# ------------------------------
# IoTropolis Makefile (Qt6, clean, automatic MOC)
# ------------------------------

CXX = g++
CXXFLAGS = -Wall -std=c++17 -fPIC `pkg-config --cflags Qt6Core Qt6Network Qt6Widgets`
LDFLAGS  = `pkg-config --libs Qt6Core Qt6Network Qt6Widgets`
MOC      = /usr/lib/qt6/libexec/moc
INCLUDES = -I./include -I./include/registration -I./include/gui
BUILD_DIR = build
TARGET = iotropolis

# ------------------------------
# Source files
# ------------------------------
SRCS = src/main.cpp \
       src/registration/IoTropolisRegistrationServer.cpp \
       src/registration/IoTropolisUnitConnection.cpp \
       src/gui/IoTropolisGui.cpp

# ------------------------------
# Headers with Q_OBJECT (for MOC)
# ------------------------------
MOC_HEADERS = include/registration/IoTropolisRegistrationServer.h \
              include/registration/IoTropolisUnitConnection.h \
              include/gui/IoTropolisGui.h

# ------------------------------
# Object files
# ------------------------------
OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/registration/IoTropolisRegistrationServer.o \
       $(BUILD_DIR)/registration/IoTropolisUnitConnection.o \
       $(BUILD_DIR)/gui/IoTropolisGui.o \
       $(BUILD_DIR)/moc/moc_IoTropolisRegistrationServer.o \
       $(BUILD_DIR)/moc/moc_IoTropolisUnitConnection.o \
       $(BUILD_DIR)/moc/moc_IoTropolisGui.o

# ------------------------------
# Default target
# ------------------------------
all: $(TARGET)

# ------------------------------
# Link executable
# ------------------------------
$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

# ------------------------------
# Compile normal source files
# ------------------------------
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------
# Generate MOC sources
# ------------------------------
$(BUILD_DIR)/moc/moc_IoTropolisRegistrationServer.cpp: include/registration/IoTropolisRegistrationServer.h
	@mkdir -p $(dir $@)
	$(MOC) -o $@ $<

$(BUILD_DIR)/moc/moc_IoTropolisUnitConnection.cpp: include/registration/IoTropolisUnitConnection.h
	@mkdir -p $(dir $@)
	$(MOC) -o $@ $<

$(BUILD_DIR)/moc/moc_IoTropolisGui.cpp: include/gui/IoTropolisGui.h
	@mkdir -p $(dir $@)
	$(MOC) -o $@ $<

# ------------------------------
# Compile MOC-generated sources into object files
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
