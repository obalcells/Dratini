OBJECT_FILES = \
	data.o \
	gen.o \
	move.o \
	hash.o \
	eval_tscp.o \
	board.o \
  	search.o \
	main.o

all: engine

engine: $(OBJECT_FILES)
	g++ -O3 -std=c++17 -o engine $(OBJECT_FILES)

%.o: %.c data.h
	g++ -O3 -std=c++17 -c $< -o $@

clean:
	rm -f *.o
	rm -f engine
