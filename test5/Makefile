OPTS = -std=c++17  -g
INCLUDE = .
LIBS = 



all: dnstest

dnstest: dnstest.cpp 
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@
	
run: dnstest
	chmod +x ./run.sh
	./run.sh

clean:
	rm -f dnstest
