OPTS = -std=c++17

all: contacts

contacts: contacts.cpp
	@g++ $(OPTS) $^ -o $@
	@chmod +x $@

clean:
	rm -f contacts
