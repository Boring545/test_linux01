OPTS = -std=c++17 -pthread -g

all: mythread

mythread: mythread.cpp threadpool.h
	@g++ $(OPTS) $^ -o $@
	@chmod +x $@
	
run: mythread
	@./mythread

clean:
	rm -f mythread
