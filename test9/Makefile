OPTS = -std=c++17 -pthread  -g
INCLUDE = .
LIBS = 



all:  coroutine

coroutine: coroutine.cpp 
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@

clean:
	rm -f coroutine 
