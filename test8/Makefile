OPTS = -std=c++17 -pthread  -g
INCLUDE = .
LIBS = 



all:  reactor

tcpserver: tcpserver.cpp ../test3/threadpool.h 
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@
reactor: reactor.cpp webserver.cpp websocket.cpp ../test3/threadpool.h webserver.h websocket.h
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@


clean:
	rm -f tcpserver reactor
