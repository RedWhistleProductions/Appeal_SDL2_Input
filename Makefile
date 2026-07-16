PLUGIN_NAME := Appeal_SDL2_Input
SRC_DIR := src
OBJ_ROOT := obj
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall
LDFLAGS ?= -shared

ifeq ($(OS),Windows_NT)
  PLATFORM := windows
  TARGET := $(PLUGIN_NAME).dll
  DEPS_ROOT ?= .deps/windows-x64
  SDL_INCLUDE ?= $(DEPS_ROOT)/include
  SDL_LIB ?= $(DEPS_ROOT)/lib
  CPPFLAGS += -I$(SDL_INCLUDE) -I$(SDL_INCLUDE)/SDL2
  LDFLAGS += -L$(SDL_LIB)
  LDLIBS += -lSDL2
else
  PLATFORM := linux
  TARGET := $(PLUGIN_NAME).so
  SDL2_CONFIG ?= sdl2-config
  CPPFLAGS += $(shell $(SDL2_CONFIG) --cflags)
  CXXFLAGS += -fPIC
  LDLIBS += $(shell $(SDL2_CONFIG) --libs) 
endif

OBJ_DIR := $(OBJ_ROOT)/$(PLATFORM)
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: check-dependencies $(TARGET)

ifeq ($(OS),Windows_NT)
check-dependencies:
	@for header in SDL.h; do test -f "$(SDL_INCLUDE)/SDL2/$$header" || { echo "Missing $(SDL_INCLUDE)/SDL2/$$header"; exit 1; }; done
	@for library in libSDL2.dll.a; do test -f "$(SDL_LIB)/$$library" || { echo "Missing $(SDL_LIB)/$$library"; exit 1; }; done
else
check-dependencies:
	@$(SDL2_CONFIG) --version >/dev/null
endif

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_ROOT) $(PLUGIN_NAME).dll $(PLUGIN_NAME).so

.PHONY: all check-dependencies clean