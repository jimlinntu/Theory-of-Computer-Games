.PHONY: clean b04705003 ucb_winlose ucb_winlose_wtree mcts mcts_alphabeta mcts_wpropru mcts_rave build_o clean

REMOVE_O = rm -f *.o
OP = -std=c++11 -O2 -Wall -Wno-unused-result

b04705003: build_o
	g++ $(OP) -DPROPRU mcts.cc einstein.o -o b04705003
	$(REMOVE_O)

# Win lose version without tree
ucb_winlose: build_o
	g++ $(OP) ucb_winlose.cc einstein.o -o ucb_winlose
	$(REMOVE_O)

# Win lose version with tree
ucb_winlose_wtree: build_o
	g++ $(OP) -DTREESEARCH ucb_winlose.cc einstein.o -o ucb_winlose_wtree
	$(REMOVE_O)

# 12~-12 score with tree
mcts: build_o
	g++ $(OP) mcts.cc einstein.o -o mcts
	$(REMOVE_O)


# 12 ~ -18 score with tree and progressive pruning
mcts_wpropru: build_o
	g++ $(OP) -DPROPRU mcts.cc einstein.o -o mcts_wpropru
	$(REMOVE_O)

# 12 ~ -18 score with tree and RAVE
mcts_rave: build_o
	g++ $(OP) -DRAVE mcts.cc einstein.o -o mcts_rave
	$(REMOVE_O)

mcts_alphabeta: build_o
	g++ $(OP) -DALPHABETA mcts.cc einstein.o -o mcts_alphabeta
	$(REMOVE_O)
	
build_o:
	g++ $(OP) einstein.cc -c
	# g++ $(OP) game.cc agent.cc getch.cc einstein.o -o game

clean:
	rm -f game greedy random
