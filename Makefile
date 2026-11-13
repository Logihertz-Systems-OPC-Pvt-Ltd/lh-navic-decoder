# Copyright 2026 Logihertz Systems (OPC) Private Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Werror -pedantic -Iinclude

SRCDIR := src
INCDIR := include
BUILDDIR := build
BINDIR := bin
TESTDIR := tests

TARGET := $(BINDIR)/lh-navic
TEST_TARGET := $(TESTDIR)/test_runner

SOURCES := $(SRCDIR)/prn_correlator.cpp \
           $(SRCDIR)/telemetry_decoder.cpp \
           $(SRCDIR)/json_parser.cpp \
           $(SRCDIR)/navic_auditor.cpp \
           $(SRCDIR)/exporters.cpp

OBJECTS := $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))
MAIN_OBJ := $(BUILDDIR)/main.o
TEST_OBJ := $(BUILDDIR)/test_runner.o

.PHONY: all test clean

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJECTS) $(MAIN_OBJ) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET): $(OBJECTS) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/test_runner.o: $(TESTDIR)/test_runner.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(BUILDDIR) $(BINDIR) $(TEST_TARGET) dist
