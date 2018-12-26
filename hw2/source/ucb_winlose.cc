#include<iostream>
#include<fstream>
#include<algorithm>
#include<random>
#include<chrono>
#include<array>
#include<cassert>
#include<thread>
#include<cmath> // https://en.cppreference.com/w/cpp/header/cmath#Special_mathematical_functions
#include"einstein.hh"


using namespace std;

class SearchEngine{
public:
    using Move = pair<int, int>;
    using LL = long long int;
    static double c;
    static LL per_simulation_count;
    ofstream debug_stream;
    // Move = a pair of (no, dir)
    SearchEngine(){
    };
    ~SearchEngine(){
    };
    // Hidden class
    struct BoardNode{
        LL wcount=0, lcount=0; // from the view of Max node
        // TODO: We only need to hash the last part of the Board class(cube position)
        // TODO: Remember that we need to gather all information from all childs before back propagation.
        // TODO: Remember MAX, MIN node are different when using UCT
        // TODO: What do I need to save in a node????
        Board now_brd;
        BoardNode *parent_node = nullptr; // point to parent brd (if it is root(now board position), it will be zero )
        // array<Move, 18> move_list = {}; 
        BoardNode *child_brds[18] = {nullptr}; // for each Cube(1,2,3,4,5,6), it can have at most three direction to move, at most 18 * 3 * 64 bit = 432 bytes
        vector<Move> move_list; 
        BoardNode() = delete;
        BoardNode(const Board &now_brd){
            this->now_brd = now_brd;
            move_list.reserve(18); // reserve max direction size
        }
        // construct child node
        BoardNode(const Board &now_brd, BoardNode * const parent_node): BoardNode(now_brd) {
            // set paranet node
            this->parent_node = parent_node;
        }
        ~BoardNode(){
            // TODO: recursively free
            for(int i = 0; i < 18; i++){
                if(child_brds[i] == nullptr){
                    break;
                }else{
                    // this will call childs destructor and cause recursive delete
                    delete child_brds[i];
                }
            }
        }
        // get total childs under itself(including itself)
        int getTotalChilds(){
            int total_child = 0;
            bool isleaf = false;
            for(int i = 0; i < 18; i++){
                if(this->child_brds[i] != nullptr){
                    // recursively sum
                    total_child += this->child_brds[i]->getTotalChilds();
                }else break;
            }
            // add itself
            return total_child+1;
        }
        double UCB_i(int child_index, int parent_depth_from_root){
            // TODO: Add static N to speed up
            double N = (double)(this->wcount + this->lcount);
            double N_i = (double)(this->child_brds[child_index]->wcount + this->child_brds[child_index]->lcount);
            // Max node
            if(parent_depth_from_root % 2 == 0){
                return (double)this->child_brds[child_index]->wcount / N_i \
                + SearchEngine::c * sqrt(log(N) / N_i);
            }
            // Min node
            else{
                return (double)this->child_brds[child_index]->lcount / N_i \
                + SearchEngine::c * sqrt(log(N) /  N_i);
            }
        }
        // Select principle variation
        BoardNode* select(){ 
            /*
                Note that this function will *always* called by root node
            */
            BoardNode *now_node_ptr = this; // now node
            BoardNode *next_node_ptr = nullptr; // next max/min node(will go over loop)
            bool isleaf = false;
            double tmp_child_ucb_score;
            // 
            for(int depth = 0;;depth++){
                //  Search for max/min node
                double max_winning_rate = -1;
                for(int i = 0; i < 18; i++){
                    if(now_node_ptr->child_brds[i] == nullptr){
                        // if this node has no child(which means it is a leaf node)
                        if(i==0){
                            isleaf = true;
                        }
                        break;
                    }
                    // child node should be simulated before
                    assert(now_node_ptr->child_brds[i]->wcount + now_node_ptr->child_brds[i]->lcount > 0);
                    // Take max over child UCB score
                    tmp_child_ucb_score = now_node_ptr->UCB_i(i, depth);
                    if(max_winning_rate < tmp_child_ucb_score){
                        max_winning_rate = tmp_child_ucb_score;
                        next_node_ptr = now_node_ptr->child_brds[i];
                    }
                }
                // if it is not a leaf, then we will keep searching
                if(!isleaf){
                    // Move to next
                    now_node_ptr = next_node_ptr;
                }else{
                    break;
                }
            }
            // now_node_ptr should not be nullptr
            assert(now_node_ptr != nullptr);
            // return pv
            return now_node_ptr;
        }
        // Expand node according to available move list
        int expand(){
            // Save legal next move
            this->move_list = this->now_brd.move_list();
            // Expand node
            Board tmp_board; // tmp board
            for(auto i = 0; i < this->move_list.size(); i++){
                // already expanded
                if(this->child_brds[i] != nullptr){
                    continue;
                }
                // restore board
                tmp_board = this->now_brd;
                // do move
                tmp_board.do_move(this->move_list[i].first, this->move_list[i].second);
                // expand node
                try{
                    this->child_brds[i] = new BoardNode(tmp_board, this);
                }catch(bad_alloc& ba){
                    cerr << "[*] bad allocate \'BoardNode\' exception" << "\n"; 
                }catch(exception& e){
                    cerr << e.what() << "\n";
                }
            }
            return this->move_list.size();
        }
        // Given a board node, perform simulation over his child nodes and return statistic result
        pair<LL,LL> simulate(const Color &mycolor){
            pair<LL, LL> w_and_l_count_pair(0, 0); // first is win, second is lose
            for(auto i = 0; i < this->move_list.size(); i++){
                assert(this->child_brds[i] != nullptr); // available move should not have nullptr child
                // playout
                for(int _ = 0; _ < SearchEngine::per_simulation_count; _++){
                    // if Max node win
                    if(this->child_brds[i]->_playout(mycolor)){
                        w_and_l_count_pair.first++;
                    }
                    // if Max node lose
                    else{
                        w_and_l_count_pair.second++;
                    }
                }
            }
            // accumulate into this node
            this->wcount += w_and_l_count_pair.first;
            this->lcount += w_and_l_count_pair.second;
            return w_and_l_count_pair;
        }
        // perform one playout on 'this' node
        bool _playout(const Color &mycolor){
            // play until one wins
            Color winner;
            Board tmp_board = this->now_brd;
            vector<Move> ml; ml.reserve(18);
            Move tmp_move;
            while(true){
                // if one wins
                winner = tmp_board.winner();
                if(winner != Color::other){
                    break;
                }
                // get move list
                ml = tmp_board.move_list();
                // uniformly randomly pick a move
                tmp_move = ml[SearchEngine::uniform(0, static_cast<int>(ml.size())-1)];
                tmp_board.do_move(tmp_move.first, tmp_move.second);
            }
            // Accumulate result in his own node and report to parent
            if(winner == mycolor){
                this->wcount++;
                return true;
            }
            else{ 
                this->lcount++;
                return false;
            }
        }

        // Back propagation score through all ancestor.
        void backprop(const pair<LL, LL> &increment_result){
            // Break if nothing need to increment
            if(increment_result.first == 0 and increment_result.second == 0){
                return;
            }
            //
            BoardNode *now_node = this->parent_node; // start from parent node
            while(true){
                // if it is a root node's virtual parent(nullptr), then stop
                if(now_node == nullptr){
                    break;
                }
                // if it is a physical node, then record statistics
                else{
                    now_node->wcount += increment_result.first;
                    now_node->lcount += increment_result.second;
                }
                // move to parent node
                now_node = now_node->parent_node;
            }
        }
        // Select best move(return index) given this node
        Move select_best_move(){
            int max_index = -1;
            double max_winning_rate = -1;
            for(auto i = 0; i < this->move_list.size(); i++){
                assert(this->child_brds[i] != nullptr);
                assert(this->child_brds[i]->wcount + this->child_brds[i]->lcount != 0);
                if(max_winning_rate < (double)this->child_brds[i]->wcount \
                    / (double)(this->child_brds[i]->wcount + this->child_brds[i]->lcount)){
                    max_winning_rate = (double)this->child_brds[i]->wcount \
                    / (double)(this->child_brds[i]->wcount + this->child_brds[i]->lcount);
                    max_index = i;
                }
            }
            assert(max_index != -1); 
            // Report best move result
            return this->move_list[max_index];
        }
    };
    // Static function bind to this SearchEngine class
    static int uniform(int l, int r){
        // initial the seed
        static random_device rd;
        // static mt19937_64 pseudo_random_engine(rd());
        static mt19937_64 pseudo_random_engine(10);
        uniform_int_distribution<int> unif(l, r);
        return unif(pseudo_random_engine);
    }
    // MCTS
    Move mcts(const Board &root_b){
        // 
        auto begin = chrono::steady_clock::now();
        LL rounds = 0;
        // Setup root board to prepare for searching
        BoardNode root_node(root_b);
        Color mycolor = root_b.turn(); // my color
        BoardNode *now_node = nullptr;
        int expand_nodes;
        
        // loop until timeout
        for(;chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - begin).count() < 8.0;){
            try{
                #ifdef TREESEARCH
                    // Selection: choose principle variation
                    now_node = root_node.select();
                    if(rounds == 0){
                        assert(now_node == &root_node); // first round should have same
                    }
                    // Expansion
                    expand_nodes = now_node->expand();
                    // Simulation
                    auto increment_result = now_node->simulate(mycolor);
                    // Backprop
                    now_node->backprop(increment_result);
                #else
                    // always 
                    expand_nodes = root_node.expand();
                    auto increment_result = root_node.simulate(mycolor);
                    root_node.backprop(increment_result); // actually do nothing
                #endif
            }catch(exception& e){
                cerr << e.what() << "\n";
            }
            rounds += expand_nodes;
        }
        // leave 0.5 seconds to let destructor free memory
        // choose best move 
        #ifdef VERBOSE
        cerr << "Total Childs: " << root_node.getTotalChilds() << "\n";
        cerr << "MCTS total rounds: " << rounds << "\n";
        cerr << "MCTS total simulation number: " << rounds * this->per_simulation_count << "\n";
        #endif
        return root_node.select_best_move();
    }
    friend ostream& operator<<(ostream& os, const SearchEngine& se){
        return os << "================ SearchEngine ============" << "\n" \
        << "UCB coefficient: " << se.c << "\n" \
        << "Per simulation count: " << se.per_simulation_count << "\n"\
        << "==========================================" << "\n";
    } 
};

// Set up static member 
double SearchEngine::c = 1.18;
long long SearchEngine::per_simulation_count = 1000;


int main(){
    //
    #ifdef VERBOSE
    cerr << SearchEngine::c << "\n";
    cerr << SearchEngine::per_simulation_count << "\n";
    #endif
    // Monte Carlo Tree Search Engine
    SearchEngine search_engine{};
    while(1){
        char player;
        cin >> player;
        if(player == 'e') // end game
            break;
        char arr[7]{};
        for(int i=0; i<=5; ++i)
            cin >> arr[i];
        Board b(arr); // initialize the board
        for(bool first_ply = (player=='f');; first_ply = false){
            // Receive oppoent's move
            if(!first_ply){
                char prev_no, prev_dir;
                cin >> prev_no >> prev_dir;
                if(prev_no == 'w' or prev_no == 'l') // win or lose
                    break;
                b.do_move(prev_no-'0', prev_dir-'0');
            }
            // Do move
            auto ml = b.move_list();
            if(ml.size() == 1ull){
                b.do_move(ml[0].first, ml[0].second);
                cout << ml[0].first << ml[0].second << flush;
                continue;
            }
            
            auto move = search_engine.mcts(b);
            
            b.do_move(move.first, move.second); // do a move
            cout << move.first << move.second << flush; // print the move
        }
    }
    return 0;
}





