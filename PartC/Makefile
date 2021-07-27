# Makefile for TCP project

all: node example

node: select.cpp node.cpp
	g++ -o node select.cpp node.cpp

example: example.cpp
	g++ -o example example.cpp select.cpp

clean:
	rm -f *.o example node