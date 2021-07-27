# Makefile for TCP project

all: server client server-ipv6 client-ipv6

server: Udp-Client.cpp
	g++ -o udpserver Udp-Server.cpp

server-ipv6: Udp-Client.cpp
	g++ -o udpserveripv6 Udp-Server-IPV6.cpp

client: Udp-Server.cpp
	g++ -o udpclient Udp-Client.cpp

client-ipv6: Udp-Server.cpp
	g++ -o udpclientipv6 Udp-Client-IPV6.cpp

clean:
	rm -f *.o udpclient udpserver udpserveripv6 udpclientipv6