all: word_counter

word_counter: count_word.cpp
	@g++ $^ -o $@
	@chmod +x $@

clean:
	rm -f word_counter
