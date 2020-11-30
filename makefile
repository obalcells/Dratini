# compiler flags
C_FLAGS = -g -w -Wfatal-errors -pipe -O3 -fno-rtti -finline-functions -fprefetch-loop-arrays
TESTING_C_FLAGS = -g -w -pipe -O1
SELF_PLAY_FLAGS = $(C_FLAGS) -DSELF_PLAY -DMAX_DEPTH=4

# link options
L_FLAGS = -s -lm
TESTING_L_FLAGS = -lm

# define output name and settings file
EXE = /Users/balce/Desktop/Dratini/dratini.sh
TEST_EXE = /Users/balce/Desktop/Dratini/test.sh
SELF_PLAY_EXE = /Users/balce/Desktop/Dratini/self_play.sh

# group of files to be compiled
SRC_FILES := $(wildcard src/*.cpp)
# take out bitboard.cpp because it's still under development
SRC_FILES := $(filter-out src/bitboard.cpp, $(SRC_FILES)) 

TEST_FILES := $(filter-out src/main.cpp, $(SRC_FILES))
#take out self_play.cpp because it's not a catf
TEST_FILES := $(filter-out test/self_play.cpp, $(SRC_FILES))
TEST_FILES += $(wildcard test/*.cpp)

SELF_PLAY_FILES := $(filter-out src/main.cpp, $(SRC_FILES)) test/self_play.cpp

default: build

build:
	@echo "Building executable"
	g++ $(L_FLAGS) $(C_FLAGS) -std=c++17 $(SRC_FILES) -o $(EXE)
	rm -rf $(EXE).dSYM

unit-tests:
	@echo "Building unit tests"
	g++ $(TESTING_L_FLAGS) $(TESTING_C_FLAGS) -std=c++17 $(TEST_FILES) -o $(TEST_EXE)
	rm -rf $(TEST_EXE).dSYM

self-play:
	@echo "Building self-play executable"
	g++ $(L_FLAGS) $(SELF_PLAY_FLAGS) -std=c++17 $(SELF_PLAY_FILES) -o $(SELF_PLAY_EXE)
	rm -rf $(SELF_PLAY_EXE).dSYM

clean:
	rm -f *.sh
	rm -rf *.dSYM