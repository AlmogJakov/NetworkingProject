# Makefile for TCP project

all: node

node: select.cpp node.cpp
	g++ -o node select.cpp node.cpp

clean:
	rm -f *.o node