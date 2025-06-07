OPTS = -std=c++17  -g
INCLUDE = .
LIBS = 



all: httptest

dnstest: httptest.cpp 
	g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	chmod +x $@
	
run: httptest
	./httptest

clean:
	rm -f httptest
