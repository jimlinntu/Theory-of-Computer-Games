#include<iostream>
#include<fstream>
#include<algorithm>
#include<random>
#include<chrono>
#include<array>
#include<cassert>
#include<thread>
#include<cmath> // https://en.cppreference.com/w/cpp/header/cmath#Special_mathematical_functions
#include<valarray>
#include <string.h>
#include <limits>
#include <memory>
#include"einstein.hh"
// #define DEBUG // FIXME: Remove it

#define MIN(a,b) (((a)<(b))?(a):(b))
using namespace std;

class SearchEngine{
public:
    using Move = pair<int, int>;
    using LL = long long int;
    static double c, c1, c2;
    static LL per_simulation_count;
    static double r_d;
    static double sigma_e;
    static double alpha;
    ofstream debug_stream;
    // Move = a pair of (no, dir)
    SearchEngine(){
    };
    ~SearchEngine(){
    };
    // Hidden class
    struct BoardNode{
        LL playout_count=0;
        LL playout_sum=0; // From the view of Max node
        LL playout_square_sum=0; // From the view of Max node
        #ifdef RAVE
        LL fake_playout_count=0;
        LL fake_playout_sum=0;
        LL fake_playout_square_sum=0;
        #endif
        // TODO: We only need to hash the last part of the Board class(cube position)
        Board now_brd;
        BoardNode *parent_node = nullptr; // point to parent brd (if it is root(now board position), it will be zero )
        // array<Move, 18> move_list = {}; 
        BoardNode *child_brds[18] = {nullptr}; // for each Cube(1,2,3,4,5,6), it can have at most three direction to move, at most 18 * 3 * 64 bit = 432 bytes
        // progressive pruning(state transition to resolve double pruned issue) (0(not mask) -> 1(mask but haven't pruned) -> 2(mask and pruned)) 
        short int child_mask[18] = {0}; 
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
            // recursively free
            for(size_t i = 0; i < this->move_list.size(); i++){
                // if it is tag as nullptr, then we will assume it is deleted
                if(child_brds[i] == nullptr){
                    continue;
                }else{
                    // this will call childs destructor and cause recursive delete
                    delete child_brds[i];
                }
            }
        }
        // get total childs under itself(including itself)
        int getTotalChilds(bool count_only_unpruned){
            int total_child = 0;
            for(int i = 0; i < 18; i++){
                if(this->child_brds[i] != nullptr){
                    // recursively sum
                    if(count_only_unpruned){
                        // if not pruned
                        if(this->child_mask[i] == 0){
                            total_child += this->child_brds[i]->getTotalChilds(count_only_unpruned);       
                        }
                    }
                    else total_child += this->child_brds[i]->getTotalChilds(count_only_unpruned);
                }else break;
            }
            // add itself
            return total_child+1;
        }
        static LL _boardscore(const Color &mycolor, const Color &opponent_color, const Board &board){
            #ifdef DEBUG
                // Should not be called by unended board
                assert(board.winner() != Color::other);
            #endif
            // #
            return (LL)(((board.winner() == mycolor)? (6):(-12)) + board.num_cubes(mycolor) - board.num_cubes(opponent_color));
        }

        double UCB_i(int child_index, int parent_depth_from_root){
            /*
                if parent_depth_from_root is even --> Max node
                if parent_depth_from_root is odd --> Min node 
                Score will be in the range of [-18, 12]
            */
            double N = (double)(this->playout_count);
            double N_i = (double)(this->child_brds[child_index]->playout_count);
            double log_N_over_N_i = log(N) / N_i ;
            // Max node: E[X] = \sum_i x_i / N_i
            // Min node: E[-X] = E[X]
            double mu_i = (double)this->child_brds[child_index]->playout_sum / N_i;
            // Max node: Var(X) = E[X^2] - (E[X])^2
            // Min node: Var(-X) = E[(-X)^2] - (E[-X])^2 = Var(X)
            // FIXME: Biased variance here
            double var = (double)this->child_brds[child_index]->playout_square_sum / N_i - pow(mu_i, 2);
            // Variance upper bound confidence
            double V_i = var + SearchEngine::c1 * sqrt(log_N_over_N_i);

            // TODO: Only merge fake mu_i
            #ifdef RAVE
                double fake_N_i = (double)this->child_brds[child_index]->fake_playout_count;
                double fake_mu_i = (double)this->child_brds[child_index]->fake_playout_sum / fake_N_i;
                if(fake_N_i == 0) fake_mu_i=0;
                double alpha = MIN(1, N_i / 10000);
                // Linear combination
                #ifdef VERBOSE
                    // cerr << "fake_N_i: " << fake_N_i << " mu_i: " << mu_i << " fake_mu_i" << fake_mu_i << "\n";
                #endif
                //
                if(fake_N_i != 0){
                    mu_i = alpha * mu_i + (1-alpha) * fake_mu_i;
                }
            #endif 

            // Max node
            if(parent_depth_from_root % 2 == 0){
                return mu_i + SearchEngine::c * sqrt(log_N_over_N_i * MIN(V_i, SearchEngine::c2));
            }
            // Min node
            else{
                return -mu_i + SearchEngine::c * sqrt(log_N_over_N_i * MIN(V_i, SearchEngine::c2));
            }
        }
        valarray<LL> progressive_pruning(bool ismax_node){
            double N_i;
            double N_j;
            double mu_i, mu_j;
            double std_i, std_j;
            double s = (ismax_node)? (+1):(-1); // if max node, sign = +1. Otherwise, -1
            int move_list_size = this->move_list.size();
            bool ispruned;
            // O(max_move^2) pruning
            while(true){
                ispruned = false;
                for(int i = 0; i < move_list_size; i++){
                    // if mask is true, then continue to consider next child
                    if(this->child_mask[i] > 0) continue;
                    
                    // Child i
                    N_i = (double)this->child_brds[i]->playout_count;
                    mu_i = s * (double)this->child_brds[i]->playout_sum / N_i;
                    std_i = sqrt((double)this->child_brds[i]->playout_square_sum / N_i - pow(mu_i, 2));
                    
                    // if std haven't converge
                    if(std_i > SearchEngine::sigma_e) continue;

                    for(int j = i+1; j < move_list_size; j++){
                        if(this->child_mask[j] > 0) continue;
                        // Child j
                        N_j = (double)this->child_brds[j]->playout_count;
                        mu_j = s * (double)this->child_brds[j]->playout_sum / N_j;
                        std_j = sqrt((double)this->child_brds[j]->playout_square_sum / N_j - pow(mu_j, 2));
                        // if these two child have not converged
                        if(std_j > SearchEngine::sigma_e) continue;
                        // Prune one child
                        if(mu_j < mu_i){
                            if((mu_j + SearchEngine::r_d * std_j) < (mu_i - SearchEngine::r_d * std_i)){
                                // prune j child
                                this->child_mask[j] = 1;
                                ispruned = true;
                            }
                        }else if(mu_i < mu_j){
                            if((mu_i + SearchEngine::r_d * std_i) < (mu_j - SearchEngine::r_d * std_j)){
                                // prune i child
                                this->child_mask[i] = 1;
                                ispruned = true;
                            }
                        }
                    }
                }
                // if no child should be pruned
                if(!ispruned){
                    break;
                }
            }
            // Gathering pruning result
            valarray<LL> decrement_result{0, 0, 0};
            // gather pruned information from child
            for(int i = 0; i < move_list_size; i++){
                #ifdef DEBUG
                    // Because we only prune newly expanded node, this should not happen
                    // TODO: But maybe it will be used on old expanded node
                    assert(this->child_mask[i] != 2);
                #endif
                // If child mask is in mask but not pruned state -> prune it and tag it as mask and pruned state
                if(this->child_mask[i] == 1){
                    // State transition
                    this->child_mask[i] = 2;
                    
                    //  Postorderly gather information for later use
                    decrement_result[0] -= this->child_brds[i]->playout_count;
                    decrement_result[1] -= this->child_brds[i]->playout_sum;
                    decrement_result[2] -= this->child_brds[i]->playout_square_sum;
                }
            }
            // Batch update on 'this' node (because backprop will only update parent node, we should update this node here)
            this->playout_count += decrement_result[0];
            this->playout_sum += decrement_result[1];
            this->playout_square_sum += decrement_result[2];
            // Report information
            return decrement_result;
        }
        // Select principle variation
        BoardNode* select(){ 
            /*
                Note that this function will *always* called by root node
            */
            #ifdef DEBUG
                assert(this->parent_node == nullptr);
            #endif
            // Declare static local to speed up
            BoardNode *now_node_ptr = this;
            BoardNode *next_node_ptr = nullptr;
            bool isleaf = false;
            double tmp_child_ucb_score;
            double max_winning_rate;
            double ninf = numeric_limits<double>::lowest();
            // 
            for(int depth = 0;;depth++){
                #ifdef DEBUG
                    assert(now_node_ptr != nullptr);
                #endif
                //  Search for max/min node
                max_winning_rate = ninf;
                for(int i = 0; i < 18; i++){
                    if(now_node_ptr->child_brds[i] == nullptr){
                        if(i == 0) isleaf = true;
                        break;
                    }
                    // this node is pruned, go to find next child
                    if(now_node_ptr->child_mask[i] > 0){
                        continue;
                    }
                    // child node should be simulated before
                    #ifdef DEBUG
                        // playout count should be greater than zero
                        assert(now_node_ptr->child_brds[i]->playout_count > 0);
                    #endif
                    // Take max over child UCB score
                    tmp_child_ucb_score = now_node_ptr->UCB_i(i, depth);
                    if(max_winning_rate < tmp_child_ucb_score){
                        max_winning_rate = tmp_child_ucb_score;
                        next_node_ptr = now_node_ptr->child_brds[i];
                    }
                }
                if(!isleaf){
                    // Move to child
                    now_node_ptr = next_node_ptr;
                }else{
                    break;
                }
            }
            // now_node_ptr should not be nullptr
            #ifdef DEBUG
                assert(now_node_ptr != nullptr);
            #endif
            // return pv
            return now_node_ptr;
        }
        // Expand node according to available move list
        int expand(){
            Board tmp_board; // tmp board
            // Save legal next move
            this->move_list = this->now_brd.move_list();
            size_t move_list_size = this->move_list.size();
            // Expand node
            for(size_t i = 0; i < move_list_size; i++){
                // The child board should be nullptr(which means expand should not be called by non-leaf node)
                assert(this->child_brds[i] == nullptr); 
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
            // return expanded node size
            return static_cast<int>(move_list_size);
        }
        // Given a board node, perform simulation over his child nodes and return statistic result
        valarray<LL> simulate(const Color &mycolor, const Color &opponent_color){
            // TODO: Silently exapnd all node
            valarray<LL> count_sum_square_sum{0, 0, 0}; // count, sum, square sum
            for(size_t i = 0; i < 18; i++){
                if(this->child_brds[i] == nullptr){
                    if(i == 0){
                        // If this node is end node, pretend simulate this node itself
                        // add `per_simulation_count`
                        // add `score` * per_simulation_count
                        // add ``
                        count_sum_square_sum[0] += SearchEngine::per_simulation_count;
                        LL score = BoardNode::_boardscore(mycolor, opponent_color, this->now_brd);
                        count_sum_square_sum[1] += score * SearchEngine::per_simulation_count;
                        count_sum_square_sum[2] += (LL)pow(score, 2) * SearchEngine::per_simulation_count;
                    }
                    break;
                }
                // Playout x times for a given node
                count_sum_square_sum += this->child_brds[i]->_playout(mycolor, opponent_color);
            }
            // accumulate into this node
            this->playout_count += count_sum_square_sum[0];
            this->playout_sum += count_sum_square_sum[1];
            this->playout_square_sum += count_sum_square_sum[2];
            

            return count_sum_square_sum;
        }
        // perform playout on 'this' node
        valarray<LL> _playout(const Color &mycolor, const Color &opponent_color){
            Color winner;
            Board tmp_board;
            vector<Move> ml; ml.reserve(18);
            int tmp_move_index;
            Move tmp_move;
            LL score, square_score;
            valarray<LL> result{0, 0, 0};
            
            #ifdef RAVE
                // Player Color x Possible Direction x Cube Number x New Square Position
                int appear_size = 2 * (3+1) * (6+1) * (25+1);
                bool appear[2][3+1][6+1][25+1] = {0};
            #endif
            // Batch add playout count
            this->playout_count += SearchEngine::per_simulation_count;
            result[0] += SearchEngine::per_simulation_count;            

            for(int _ = 0; _ < SearchEngine::per_simulation_count; _++){
                // play until one wins
                tmp_board = this->now_brd;
                while(true){
                    // if one wins
                    winner = tmp_board.winner();
                    if(winner != Color::other){
                        break;
                    }
                    // get move list
                    ml = tmp_board.move_list();
                    // uniformly randomly pick a move
                    tmp_move_index = SearchEngine::uniform(0, static_cast<int>(ml.size())-1);
                    tmp_move = ml[tmp_move_index];
                    
                    #ifdef RAVE
                        // record appearance
                        int datum = tmp_board.new_pos(tmp_move.first, tmp_move.second).getDatum();
                        // Now Player Color x Move Direction x Which Cube Number x  Square Position After Move
                        appear[(tmp_board.turn() == mycolor)][tmp_move.second][tmp_move.first][datum] = true; 
                        assert(datum < 26);
                    #endif
                    // do move
                    tmp_board.do_move(tmp_move.first, tmp_move.second);
                }
                
                // Accumulate result in his own node and report to parent
                score = BoardNode::_boardscore(mycolor, opponent_color, tmp_board);
                square_score = (LL)pow(score, 2);
                
                this->playout_sum += score;
                this->playout_square_sum += square_score;

                result[1] += score;
                result[2] += square_score;

                // AMAF fake backprop
                #ifdef RAVE
                    BoardNode *now_node_ptr = this;
                    BoardNode *tmp_parent;
                    BoardNode *tmp_sibling; // now_node_ptr's sibling
                    LL tmp_fake_playout_count=0, tmp_fake_playout_sum=0, tmp_fake_playout_square_sum=0;
                    // Backprop fake information
                    while(true){
                        tmp_parent = now_node_ptr->parent_node;
                        if(tmp_parent == nullptr){
                            break;
                        }
                        for(int i = 0; i < 18; i++){
                            tmp_sibling = tmp_parent->child_brds[i];
                            if(tmp_sibling == nullptr) break;
                            if(tmp_sibling == now_node_ptr) continue;
                            if(tmp_sibling->move_list.size() != 0) continue;

                            tmp_move = tmp_parent->move_list[i];
                            int datum = tmp_parent->now_brd.new_pos(tmp_move.first, tmp_move.second).getDatum();
                            bool isappear = appear[tmp_parent->now_brd.turn() == mycolor][tmp_move.second][tmp_move.first][datum];
                            // if appear in simulation playout
                            if(isappear){
                                // record information on sibling node
                                // pretend this sibling have this playout
                                tmp_sibling->fake_playout_count += 1;
                                tmp_sibling->fake_playout_sum += score;
                                tmp_sibling->fake_playout_square_sum += square_score;

                                // Like a reverse river stream, gather information from branch
                                tmp_fake_playout_count += 1;
                                tmp_fake_playout_sum += score;
                                tmp_fake_playout_square_sum += square_score;
                            }
                        }
                        // update on parent node
                        tmp_parent->fake_playout_count += tmp_fake_playout_count;
                        tmp_parent->fake_playout_sum += tmp_fake_playout_sum;
                        tmp_parent->fake_playout_square_sum += tmp_fake_playout_square_sum;
                        // Move to parent
                        now_node_ptr = tmp_parent;
                    }
                    // reset appear
                    memset(appear, 0, sizeof(bool) * appear_size);
                #endif 
            }
            return result;
        }
        // Back propagation score through all ancestor.
        void backprop(const valarray<LL> &increment_result){
            // Break if increment playout_count is zero
            if(increment_result[0] == 0){
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
                    now_node->playout_count += increment_result[0];
                    now_node->playout_sum += increment_result[1];
                    now_node->playout_square_sum += increment_result[2];
                }
                #ifdef DEBUG
                    assert(now_node->playout_count >= 0);
                    assert(now_node->playout_square_sum >= 0);
                #endif
                // move to parent node
                now_node = now_node->parent_node;
            }
        }
        // Select best move(return index) given this node
        Move select_best_move(){
            #ifdef VERBOSE
                cerr << "select best move\n";
            #endif
            int max_index = -1;
            double max_winning_rate = numeric_limits<double>::lowest();
            double tmp_winning_rate;
            for(int i = 0; i < 18; i++){
                if(this->child_brds[i] == nullptr){
                    #ifdef DEBUG
                        assert(i != 0);
                    #endif
                    break;
                }else{
                    #ifdef DEBUG
                        assert(this->child_brds[i]->playout_count > 0);
                    #endif
                    // Max over mean score
                    tmp_winning_rate = (double)this->child_brds[i]->playout_sum / (double)this->child_brds[i]->playout_count;
                    #ifdef VERBOSE
                        auto std = sqrt((double)this->child_brds[i]->playout_square_sum / (double)this->child_brds[i]->playout_count - pow(tmp_winning_rate, 2));
                        cerr << "Index " << i << " move's mean is: " << tmp_winning_rate <<  " std is: " << std << "\n";
                    #endif
                    if(max_winning_rate < tmp_winning_rate){
                        max_winning_rate = tmp_winning_rate;
                        max_index = i;
                    }
                }
            }
            #ifdef DEBUG
                assert(max_index != -1); 
            #endif
            #ifdef VERBOSE
                cerr << "end select best move\n";
                cerr << "Best move's score: " << max_winning_rate <<"\n";
            #endif
            // Report best move result
            return this->move_list[max_index];
        }
        // Only occur when ALPHABETA macro is defined
        Move select_best_move_by_alphabeta(const Color &mycolor){
            // for shuffle
            static mt19937_64 pseudo_random_engine(random_device{}());
            
            int max_index = -1;
            double m = numeric_limits<double>::lowest(), beta = numeric_limits<double>::max();
            vector<int> tie_indices; tie_indices.reserve(18);
            double t;
            for(size_t i = 0; i < this->move_list.size(); i++){
                t = -SearchEngine::alpha_beta(this->child_brds[i], -beta, -m, mycolor);
                #ifdef VERBOSE
                    cerr << "Move Index " << i << " score is: " << t << "\n";
                #endif
                if(t > m){
                    m = t;
                    max_index = i;
                    tie_indices.clear(); // since one bigger than other, tie is clear
                }
                // if accidentally equal, then we just do sampling
                else if(t == m){
                    tie_indices.push_back(i);
                }
            }
            if(tie_indices.size() == 0) return this->move_list[max_index];
            else{
                // shuffle
                shuffle(tie_indices.begin(), tie_indices.end(), pseudo_random_engine);
                return this->move_list[tie_indices[0]];
            }
        }

        // return remaining substree's root
        static BoardNode* do_move(BoardNode * const root_boardnode, const int no, const int dir){
            // (Won't happened in MAX node because we are sure to expand all child board nodes)
            // (Maybe happend when root_boardnode is MIN node): if haven't expanded, delete this node and return nullptr
            if(root_boardnode->move_list.size() == 0){
                // delete all existing tree
                delete root_boardnode;
                return nullptr;
            }

            BoardNode *next_boardnode = nullptr;
            int next_index = -1;
            for(size_t i = 0; i < root_boardnode->move_list.size(); i++){
                // 
                if((no == root_boardnode->move_list[i].first) && (dir == root_boardnode->move_list[i].second)){
                    // next_boarnode
                    next_boardnode = root_boardnode->child_brds[i];
                    next_index = i;
                    // mask out this child(prevent being deleted in destructor)
                    root_boardnode->child_brds[i] = nullptr;
                    break;
                }
            }

            #ifdef DEBUG
                if(root_boardnode->child_mask[next_index] > 0){
                    // it should be a leaf node(because I use hard progressive pruning)
                    assert(next_boardnode->move_list.size() == 0);
                }
            #endif
            // delete parent(recursively), but won't delete next_boardnode because we annotate it as a nullptr
            delete root_boardnode;
            // tag parent_node as nullptr
            next_boardnode->parent_node = nullptr;
            //
            return next_boardnode;
        }
    };
    // Static function bind to this SearchEngine class
    static int uniform(int l, int r){
        // FIXME: REMEBER TO CHANGE RANDOM SEED WHEN SUBMITTING
        // initial the seed
        static random_device rd;
        static mt19937_64 pseudo_random_engine(rd());
        uniform_int_distribution<int> unif(l, r);
        return unif(pseudo_random_engine);
    }
    // MCTS
    Move mcts(BoardNode * const root_node){
        // 
        auto begin = chrono::steady_clock::now();
        LL rounds = 0;
        // Setup root board to prepare for searching
        Color mycolor = root_node->now_brd.turn(); // my color
        Color opponent_color = (mycolor == Color::blue)? (Color::red):(Color::blue); // opponent color is different from me
        BoardNode *now_node = nullptr;
        int expand_nodes = 0;
        
        // loop until timeout 
        for(;chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - begin).count() < 8.5;){
            try{
                // Selection: choose principle variation
                now_node = root_node->select();
                // Expansion
                expand_nodes += now_node->expand();
                // Simulation
                auto increment_result = now_node->simulate(mycolor, opponent_color);
                // Progressive Pruning
                #ifdef PROPRU
                    auto decrement_result = now_node->progressive_pruning((mycolor == now_node->now_brd.turn()));
                    // Minus pruned node
                    increment_result += decrement_result;
                #endif
                // Backprop
                now_node->backprop(increment_result);
                // Add simulation rounds
                rounds += increment_result[0];
            }catch(exception& e){
                cerr << e.what() << "\n";
            }
        }
        // leave 0.5 seconds to let destructor free memory
        // choose best move 
        #ifdef VERBOSE
            cerr << "Total Expanded Nodes: "<< expand_nodes << "\n";
            cerr << "Total Childs: " << root_node->getTotalChilds(true) << "\n";
            cerr << "MCTS total simulation number: " << rounds << "\n";
            cerr << "playout count: " << root_node->playout_count << "\n";
            cerr << "playout sum: " << root_node->playout_sum << "\n";
            cerr << "playout square sum: " << root_node->playout_square_sum << "\n"; 
            #ifdef RAVE
                cerr << "fake playout count: " << root_node->fake_playout_count << "\n";
                cerr << "fake playout sum: " << root_node->fake_playout_sum << "\n";
                cerr << "fake playout sqaure sum: " << root_node->fake_playout_square_sum << "\n";
            #endif
        #endif
        
        #ifdef ALPHABETA
            return root_node->select_best_move_by_alphabeta(mycolor);
        #else 
            return root_node->select_best_move();
        #endif
    }

    // This function won't use member variable
    static double alpha_beta(BoardNode const *board_node, const double &alpha, const double &beta, const Color &mycolor){
        // base case
        if(board_node->move_list.size() == 0){
            // return simulation score            
            return ((board_node->now_brd.turn() == mycolor)? (1.0):(-1.0)) * (double)board_node->playout_sum / (double)board_node->playout_count;
        }

        double m = alpha;
        double t;
        for(size_t i = 0; i < board_node->move_list.size(); i++){
            t = -alpha_beta(board_node->child_brds[i], -beta, -m, mycolor);
            if(t > m){
                m = t;
                if(m >= beta){
                    return beta;
                }
            }
        }
        return m;
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
double SearchEngine::c1 = 2;
double SearchEngine::c2 = 3;
long long SearchEngine::per_simulation_count = 90;
double SearchEngine::r_d = 9;
double SearchEngine::sigma_e = 0.2;


int main(){
    #ifdef VERBOSE
        #ifdef PROPRU
            cerr << "[*] Using progressive pruning...\n";
        #endif
        #ifdef ALPHABETA
            cerr << "[*] Using Alphabeta...\n";
        #endif
        #ifdef RAVE
            cerr << "[*] Using RAVE...\n";
        #endif
        #ifndef PROPRU
            #ifndef RAVE
                cerr << "[*] Using Pure MCTS...\n";
            #endif
        #endif
    #endif
    
    #ifdef VERBOSE
    cerr << "c = " << SearchEngine::c << "\n";
    cerr << "c1 = " <<SearchEngine::c1 << "\n";
    cerr << "c2 = " << SearchEngine::c2 << "\n";
    cerr << "per simulation count = " << SearchEngine::per_simulation_count << "\n";
    cerr << "r_d = " << SearchEngine::r_d << "\n";
    cerr << "sigma_e = " << SearchEngine::sigma_e << "\n";
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
        SearchEngine::BoardNode *root_boardnode = nullptr;
        for(bool first_ply = (player=='f');; first_ply = false){
            // Receive oppoent's move
            if(!first_ply){
                char prev_no, prev_dir;
                cin >> prev_no >> prev_dir;
                if(prev_no == 'w' or prev_no == 'l'){ // win or lose
                    if(root_boardnode != nullptr){
                        // free memory
                        delete root_boardnode;
                        root_boardnode = nullptr; // tag it as nullptr
                    }
                    break;
                }
                b.do_move(prev_no-'0', prev_dir-'0');

                // if root_boardnode is not nullptr, let it move to next child
                if(root_boardnode != nullptr){
                    root_boardnode = SearchEngine::BoardNode::do_move(root_boardnode, prev_no-'0', prev_dir-'0');
                }
            }

            // initialize root board
            if(root_boardnode == nullptr){
                root_boardnode = new SearchEngine::BoardNode(b);
            }

            // Do move
            auto ml = b.move_list();
            if(ml.size() == 1ull){
                b.do_move(ml[0].first, ml[0].second);
                cout << ml[0].first << ml[0].second << flush;

                // do move
                root_boardnode = SearchEngine::BoardNode::do_move(root_boardnode, ml[0].first, ml[0].second);
                continue;
            }
            #ifdef VERBOSE
                cerr << "============== Start Searching ==============" << "\n" << flush;
            #endif 
            
            auto move = search_engine.mcts(root_boardnode);
            // delete sibling tree, return next child and assign it back to root_boardnode
            root_boardnode = SearchEngine::BoardNode::do_move(root_boardnode, move.first, move.second);
            
            #ifdef VERBOSE
                cerr << "============== End Searching ==============" << "\n" << flush;
            #endif
            b.do_move(move.first, move.second); // do a move
            cout << move.first << move.second << flush; // print the move
        }
    }
    return 0;
}






