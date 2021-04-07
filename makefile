# compiler flags
C_FLAGS = -g -w --std=c++17 -pthread -Wfatal-errors -pipe -O3 -fno-rtti -finline-functions -fprefetch-loop-arrays
TESTING_C_FLAGS = -g -w -pthread --std=c++17
SELF_PLAY_FLAGS = $(TESTING_C_FLAGS) -DSELF_PLAY -DMAX_DEPTH=4

# link options
L_FLAGS = -s -lm
TESTING_L_FLAGS = -lm

# define output name and settings file
# EXE = /Users/balce/Desktop/Dratini/dratini
EXE = /Users/balce/dratini
TEST_EXE = /Users/balce/Desktop/Dratini/test.sh
SELF_PLAY_EXE = /Users/balce/Desktop/Dratini/self_play.sh

# group of files to be compiled
SRC_FILES := $(wildcard src/*.cpp)
TEST_FILES := $(filter-out src/main.cpp, $(SRC_FILES))
TEST_FILES += $(wildcard test/*.cpp)
TEST_FILES := $(filter-out test/debug.cpp, $(TEST_FILES))
DEBUG_FILES := $(filter-out src/main.cpp, $(SRC_FILES)) test/debug.cpp

default: build 

debug:
	g++ $(TESTING_L_FLAGS) $(TESTING_C_FLAGS) $(DEBUG_FILES) -o debug.sh

build:
	@echo "Building executable"
	g++ $(L_FLAGS) $(C_FLAGS) $(SRC_FILES) -o $(EXE)

tests:
	@echo "Building unit tests"
	g++ $(TESTING_L_FLAGS) $(TESTING_C_FLAGS) $(TEST_FILES) -o $(TEST_EXE)

self-play:
	@echo "Building self-play executable"
	g++ $(L_FLAGS) $(SELF_PLAY_FLAGS) $(SELF_PLAY_FILES) -o $(SELF_PLAY_EXE)

clean:
	rm -f *.sh
	rm -rf *.dSYM
