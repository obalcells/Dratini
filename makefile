OBJECT_FILES = \
	board.o \
	data.o \
	main.o

all: obce

obce: $(OBJECT_FILES)
	g++ -O3 -o obce $(OBJECT_FILES)

%.o: %.c data.h
	g++ -O3 -c $< -o $@

clean:
	rm -f *.o
	rm -f obce
