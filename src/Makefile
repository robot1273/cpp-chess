CXX = g++
CXXFLAGS = -g -Wall -std=c++17

# find all .cpp files relative to current directory (src/)
SRC = $(shell find . -name '*.cpp')

OBJ_DIR = ../build
OBJ = $(patsubst ./%, $(OBJ_DIR)/%, $(SRC:.cpp=.o))

INCLUDE_DIRS = $(shell find . -type d)
INCLUDE_FLAGS = $(patsubst %,-I%,$(INCLUDE_DIRS))

TARGET = ../chess   # executable in project root

all: $(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
