OPTS = -std=c++17 -pthread -g
INCLUDE = /usr/include/mysql 
LIBS = -lmysqlclient



all: sqltest

sqltest: sqltest.cpp connectionpool.h
	@g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	@chmod +x $@
	
run: sqltest
	@./sqltest

clean:
	rm -f sqltest green_download.jpg 
