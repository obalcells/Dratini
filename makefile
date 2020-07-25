OBJECT_FILES = \
	data.o \
	eval.o \
	board.o \
  search.o \
	main.o

all: engine

engine: $(OBJECT_FILES)
	g++ -O3 -o engine $(OBJECT_FILES)

%.o: %.c data.h
	g++ -O3 -c $< -o $@

clean:
	rm -f *.o
	rm -f engine
