OBJECT_FILES = \
	main.o \
	tt.o \
	data.o \
	gen.o \
	move.o \
	hash.o \
	eval_tscp.o \
	board.o \
 	search.o \
	book.o \
	stats.o

TESTING_FILES = \
	test.o \
	tt.o \
	data.o \
	gen.o \
	move.o \
	hash.o \
	eval_tscp.o \
	board.o \
 	search.o \
	book.o \
	stats.o

all: engine

engine: $(OBJECT_FILES)
	g++ -O3 -std=c++17 -o engine $(OBJECT_FILES)

testing: $(TESTING_FILES)
	g++ -03 -std=c++17 -o testing_engine $(TESTING_FILES)

%.o: %.c data.h
	g++ -O3 -std=c++17 -c $< -o $@

clean:
	rm -f *.o
	rm -f engine
