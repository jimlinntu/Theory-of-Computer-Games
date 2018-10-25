.PHONY: clean
CC := g++ -std=c++11 -O3 -Wall
exe := main
c_file := main.cpp
all: $(obj)
	$(CC) -o $(exe) $(c_file)

all_debug: $(obj)
	$(CC) -o $(exe) $(c_file) -g3

clean:
	rm -f $(exe)
