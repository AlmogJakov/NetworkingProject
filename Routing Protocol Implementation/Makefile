# Makefile for TCP project

all: node

node: select.cpp main.cpp sender.cpp stdin_input.cpp sockets_input.cpp global_methods.cpp
	g++ -o node select.cpp main.cpp sender.cpp stdin_input.cpp sockets_input.cpp global_methods.cpp

clean:
	rm -f *.o node