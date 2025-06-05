OPTS = -std=c++17 -pthread -g
INCLUDE = /usr/include/mysql
LIBS = -lmysqlclient



all: sqltest

sqltest: sqltest.cpp
	@g++ $(OPTS) -I$(INCLUDE) $^ -o $@ $(LIBS)
	@chmod +x $@
	
run: sqltest
	@echo "Running ./sqltest ..."
	@./sqltest

clean:
	rm -f sqltest
