OPTS = -std=c++17 -pthread

all: mythread

mythread: mythread.cpp
	@g++ $(OPTS) $^ -o $@
	@chmod +x $@
	
run: mythread
	@echo "Running ./mythread ..."
	@./mythread

clean:
	rm -f mythread
