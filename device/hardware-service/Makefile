CXX ?= g++
CPPFLAGS ?=
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

BUILD_DIR := build
SERVICE := $(BUILD_DIR)/retropixel-server
BRIGHTNESS_TEST := $(BUILD_DIR)/brightness-policy-test
VOLUME_TEST := $(BUILD_DIR)/volume-policy-test

.PHONY: all test clean

all: $(SERVICE)

$(BUILD_DIR):
	mkdir -p $@

$(SERVICE): main.cpp VolumeController.cpp VolumeController.hpp BrightnessPolicy.hpp VolumePolicy.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) main.cpp VolumeController.cpp $(LDFLAGS) -o $@

$(BRIGHTNESS_TEST): tests/BrightnessPolicy_test.cpp BrightnessPolicy.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/BrightnessPolicy_test.cpp $(LDFLAGS) -o $@

$(VOLUME_TEST): tests/VolumePolicy_test.cpp VolumePolicy.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/VolumePolicy_test.cpp $(LDFLAGS) -o $@

test: $(BRIGHTNESS_TEST) $(VOLUME_TEST)
	./$(BRIGHTNESS_TEST)
	./$(VOLUME_TEST)

clean:
	rm -rf $(BUILD_DIR)
