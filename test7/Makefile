OPTS = -std=c++17 -pthread  -g
INCLUDE = .
LIBS = 



all: tcpserver

tcpserver: tcpserver.cpp ../test3/threadpool.h
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@
	
run: tcpserver
	./tcpserver

clean:
	rm -f tcpserver
