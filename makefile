# compiler flags
C_FLAGS = -g -w -Wfatal-errors -pipe -O3 -fno-rtti -finline-functions -fprefetch-loop-arrays
TESTING_C_FLAGS = -g -w -pipe -O1

# link options
L_FLAGS = -s -lm
TESTING_L_FLAGS = -lm

# define output name and settings file
EXE_NAME = /Users/balce/Desktop/Dratini/dratini
TEST_NAME = /Users/balce/Desktop/Dratini/test_dratini

# group of files to be compiled
SRC_FILES := $(wildcard src/*.cpp)
TEST_FILES := $(filter-out src/main.cpp, $(SRC_FILES))
TEST_FILES += $(wildcard test/*.cpp)

default: build

build:
	@echo "Building executable"
	g++ $(L_FLAGS) $(C_FLAGS) -std=c++17 $(SRC_FILES) -o $(EXE_NAME)
	rm -rf $(EXE_NAME).dSYM

tests:
	@echo "Building testing executable"
	g++ $(TESTING_L_FLAGS) $(TESTING_C_FLAGS) -std=c++17 $(TEST_FILES) -o $(TEST_NAME)
	rm -rf $(TEST_NAME).dSYM

clean:
	rm -f $(EXE_NAME)
	rm -f $(TEST_NAME)
	rm -rf $(EXE_NAME).dSYM
	rm -rf $(TEST_NAME).dSYM