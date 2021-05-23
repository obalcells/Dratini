# compiler flags
C_FLAGS = -g -w -s -lm --std=c++11 -pthread -Wfatal-errors -pipe -O3 -fno-rtti -finline-functions -fprefetch-loop-arrays 

EXE =$(shell pwd)/dratini
TEST_EXE=$(shell pwd)/test.sh
SELF_PLAY_EXE=$(shell pwd) /self_play.sh

# group of files to be compiled
SRC_FILES := $(wildcard src/*.cpp)
TEST_FILES := $(filter-out src/main.cpp, $(SRC_FILES))
TEST_FILES += $(wildcard test/*.cpp)
TEST_FILES := $(filter-out test/debug.cpp, $(TEST_FILES))
DEBUG_FILES := $(filter-out src/main.cpp, $(SRC_FILES)) test/debug.cpp test/board_speed.cpp test/sungorus_board.cpp

default: debug 

debug:
	g++ $(C_FLAGS) $(DEBUG_FILES) -o debug.sh

build:
	@echo "Building executable"
	g++ $(C_FLAGS) $(SRC_FILES) -o $(EXE)

tests:
	@echo "Building tests"
	g++ $(C_FLAGS) $(TEST_FILES) -o $(TEST_EXE)

clean:
	rm -f *.sh
	rm -rf *.dSYM
