CXX = g++
CXXFLAGS = -g -Wall -std=c++17

SRC_DIR = src
SRC = $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_DIR = build
OBJ = $(patsubst $(SRC_DIR)/%, $(OBJ_DIR)/%, $(SRC:.cpp=.o))

INCLUDE_DIRS = $(shell find $(SRC_DIR) -type d)
INCLUDE_FLAGS = $(patsubst %,-I%,$(INCLUDE_DIRS))

TARGET = chess

all: $(TARGET)

# Compile each source file into build/ while preserving directory structure
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
