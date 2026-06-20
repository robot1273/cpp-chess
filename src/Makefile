CXX = g++
# CXXFLAGS = -g -O0 -Wall -std=c++23
CXXFLAGS = -O3 -march=native -DNDEBUG -Wall -std=c++23

# find all .cpp files relative to current directory (src/)
SRC = $(shell find . -name '*.cpp')

OBJ_DIR = ../build
OBJ = $(patsubst ./%, $(OBJ_DIR)/%, $(SRC:.cpp=.o))

INCLUDE_DIRS = $(shell find . -type d)
INCLUDE_FLAGS = $(patsubst %,-I%,$(INCLUDE_DIRS))

TARGET = ../chess   # executable in project root

all: $(TARGET)

compiledb: clean
	@bear -- make $(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
	rm -f compile_commands.json

run:
	make
	../chess
