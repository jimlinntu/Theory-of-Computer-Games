LDFLAGS=-static -s -O2 -std=c++11 -Wall
.PHONY:	clean
search.exe:	main.cc anqi.cc TranspositionTable.cc ClientSocket.cpp Protocol.cpp
	$(CXX) $(LDFLAGS) -o $@ $^ 
clean:
	DEL search.exe 2>NUL
