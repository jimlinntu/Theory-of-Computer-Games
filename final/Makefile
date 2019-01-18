LDFLAGS=-static -s -O2 -std=c++11 -Wall -g -rdynamic 
.PHONY:	clean search_attackMove search
search_attackMove:	main.cc anqi.cc TranspositionTable.cc ClientSocket.cpp Protocol.cpp
	$(CXX) $(LDFLAGS) -DATTACKMOVE  -o search.exe $^

search:	main.cc anqi.cc TranspositionTable.cc ClientSocket.cpp Protocol.cpp
	$(CXX) $(LDFLAGS)  -o search.exe $^

clean:
	DEL search.exe 2>NUL
