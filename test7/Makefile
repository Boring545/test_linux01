OPTS = -std=c++17 -pthread  -g
INCLUDE = .
LIBS = 



all: tcpserver client

tcpserver: tcpserver.cpp ../test3/threadpool.h
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@

client: mul_port_client_epoll.c
	gcc $^ -o $@
	chmod +x $@

clean:
	rm -f tcpserver
	rm -f client
