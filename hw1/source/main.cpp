#include <stdio.h>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <queue>
#include <bitset>
#include <time.h>
#include <functional>
#define HEIGHT_MAX 15
#define WIDTH_MAX 15
#define OFFSET 10
#define MAX(a,b) (((a)>(b))?(a):(b))
// #define DEBUG
using namespace std;
using ULL = unsigned long long int;
// 0~5 bit for player position number(for left to right, top to down) 
// 6~9 bit for m_cnt
// 10~59 bit for box on the board
using PlayerandBox = ULL; 
// UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3
int const Di[]={-1, 1, 0, 0}, Dj[]={0, 0, -1, 1};
int edgeGoalNumber[] = {0, 0, 0, 0};
int box_number = 0;
// (player position, box position)
int h[HEIGHT_MAX*WIDTH_MAX][HEIGHT_MAX*WIDTH_MAX]; // heuristic function: if only that box exist, how many steps to finish so that m_cnt =0

int counter = 0;

const int NumberOfDirection = 4;
// UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3
typedef enum{
    UP, DOWN, LEFT, RIGHT
}Direction;
// push: whether a box is pushed in the move
struct Move{
    Direction dir;
    bool push;
    Move(){};
    Move(Direction dir, bool push){
        this->dir = dir;
        this->push = push;
    }
};
struct Board{
    int height, width;
    char board[HEIGHT_MAX][WIDTH_MAX+2]; // +2 for LF and NULL characters
    ULL wallboard; // bit represent wall(last 10 bit will be set to zero)
    ULL goalboard;
    // ==== Critial information ====
    void init(){
        this->height = 0, this->width = 0;
        memset(this->board, 0, sizeof(char) * HEIGHT_MAX * (WIDTH_MAX+2));
        this->wallboard = 0, goalboard = 0;
    }
};
Board Brd;

struct Node{
    PlayerandBox pb;
    int f; // total distance
    int g; // distance until now 
    Node(){
        f = 0, g = 0, pb = 0;
    }
    Node(int f_, int g_, PlayerandBox pb_){
        this->f = f_, this->g = g_, this->pb = pb_;
    }
    bool operator>(const Node &rhs) const{
        return (this->f > rhs.f);
    }
};
struct Visitandf{
    bool visited;
    int f;
    Visitandf(){
        visited = false;
        f = -1;
    }
    Visitandf(bool visited_, bool f_){
        this->visited = visited_;
        this->f = f_;
    }
};

using BacktrackMap = unordered_map<PlayerandBox, Move>;
using BacktrackMap_map = map<PlayerandBox, Move>;
using VisitedMap = unordered_map<PlayerandBox, bool>;
using VisitedMap_withf = unordered_map<PlayerandBox, Visitandf>;
using VisitedMap_map = map<PlayerandBox, bool>;
using PlayerandBoxSet = unordered_set<PlayerandBox>;
using Vector_Move = vector<Move>;
using PQ = priority_queue<Node, vector<Node>, greater<Node>>; // Saving f and g


// =================== Search Algorithm =============================
bool bfs(Vector_Move &History, const PlayerandBox &initial_pb);
void reverse_bfs(Vector_Move &History, const PlayerandBox &initial_pb, const vector<PlayerandBox> &goal_set);
void bibfs(Vector_Move &History, const PlayerandBox &initial_pb, const vector<PlayerandBox> &goal_set);
bool A_star(Vector_Move &History, const PlayerandBox &initial_pb);
bool A_star_rbfs(Vector_Move &History, const PlayerandBox &initial_pb, const vector<PlayerandBox> &goal_set);
// =================== Board Related ================================
bool Get_Board(FILE *const fin, PlayerandBox &initial_pb);
bool Inside(int const i, int const j);
PlayerandBox Do_move(const PlayerandBox &curr_pb, Direction const dir, Vector_Move *history, bool &ispush);
PlayerandBox reverse_Do_move(const PlayerandBox &curr_pb, Direction const dir, bool pull, Vector_Move *const reverse_history);
PlayerandBox Undo(const PlayerandBox &curr_pb, Move const mv);
PlayerandBox reverse_Undo(const PlayerandBox &curr_pb, Move const mv);
void generate_Goal_states(vector<PlayerandBox> &goal_set);
bool check_dead_push(const PlayerandBox &curr_pb, const Direction &dir);
// ==================== DEBUG =================================
void printBoard(Board *const brd, const PlayerandBox &curr_pb);

// compute heuristic
void precompute_h(){
    /*
        Assume only exist one box on board
    */
    PlayerandBox pb = 0;
    Vector_Move answer_history; answer_history.reserve(10000);
    memset(h, 0, sizeof(h));
    // put player at every place
    for(int i=0; i <Brd.width*Brd.height; i++){
        // put a box at every square place
        for(int j=0; j<Brd.width*Brd.height; j++){
            // if player or box is in wall, g(Node) = infinity
            if((!Inside(i / Brd.width, i % Brd.width)) || (!Inside(j / Brd.width, j % Brd.width))){
                h[i][j] = -1;
                continue;
            }
            if(i == j){
                h[i][j] = -1; // impossible
                continue;
            }
            // if the box is already at one goal, then g(Node) = 0
            if((Brd.goalboard & (static_cast<ULL>(1) << (j+OFFSET))) > 0){
                h[i][j] = 0;
                continue;
            }
            pb = 0;
            // set m_cnt to 1
            pb += static_cast<ULL>(1) << 6;
            // put player and box on it
            pb += static_cast<ULL>(i);
            pb |= static_cast<ULL>(1) << (j + OFFSET);
            // if we can find a solution
            answer_history.clear();
            if(bfs(answer_history, pb)){
                // Set heuristic
                h[i][j] = answer_history.size();
            }else{
                h[i][j] = -1; // infinity
            }
        }
    }
}

int compute_h(const PlayerandBox &curr_pb){
    /*
        -1 denote infinity
    */
    // 
    ULL bit = static_cast<ULL>(1) << (OFFSET);
    int player_number = curr_pb & (static_cast<ULL>(63));
    int sum_h = -1;
    // loop over all position
    for(int i=0; i<Brd.width*Brd.height; i++){
        // if there is a box
        if((curr_pb & bit) > 0){
            // if h is not infinity
            if(h[player_number][i] != -1){
                sum_h = MAX(sum_h, h[player_number][i]);
            }else{
                sum_h = -1;
                break;
            }
        }
        bit = bit << 1;
    }
    return sum_h;
}   

int main(int argc, char const *argv[]){
    // init
    Vector_Move answer_history; answer_history.reserve(10000000);
    PlayerandBox initial_pb; 
    vector<PlayerandBox> goal_set;
    
    // Fix random seed 
    srand(9487);
    // 
    assert(argc == 2);

    while(Get_Board(stdin, initial_pb)){
        #ifdef DEBUG
        printf("Original Board: \n");
        printBoard(&Brd, initial_pb);
        #endif
        // precompute!!!
        
        // =========================
        if (strcmp(argv[1], "iddfs") == 0){
            assert(0);
        }else if(strcmp(argv[1], "idbds") == 0){
            assert(0);
        }else if(strcmp(argv[1], "bfs") == 0){
            bfs(answer_history, initial_pb);
        }else if(strcmp(argv[1], "rbfs") == 0){
            generate_Goal_states(goal_set);
            reverse_bfs(answer_history, initial_pb, goal_set);
        }else if(strcmp(argv[1], "bibfs") == 0){
            generate_Goal_states(goal_set);
            bibfs(answer_history, initial_pb, goal_set);
        }else if(strcmp(argv[1], "Astar") == 0){
            precompute_h();
            A_star(answer_history, initial_pb);
        }else{
            // printf("[!] Sorry, this algorithm does not exist!\n");
            assert(0);
        }
        printf("%zd\n", answer_history.size());
        for(int i=0; i<answer_history.size(); ++i){
            putchar("udlr"[answer_history[i].dir]);
        }
        putchar('\n');
        // 
        answer_history.clear();
        goal_set.clear();
        
    }
    return 0;
}
bool A_star(Vector_Move &History, const PlayerandBox &initial_pb){
    /*
    */
    BacktrackMap backtrackmap; backtrackmap.reserve(10000000);
    VisitedMap_withf visitedmap; visitedmap.reserve(10000000);
    PlayerandBoxSet depth_limit_board_sets; //depth_limit_board_sets.reserve(10000000);
    Node curr_pb, next_pb;
    int tmp_h;
    // init queue
    PQ queue_pb;
    // insert start node into queue
    tmp_h = compute_h(initial_pb);
    assert(tmp_h != -1);
    queue_pb.push(Node(0+tmp_h, 0, initial_pb));
    visitedmap[initial_pb].visited = true;
    visitedmap[initial_pb].f = 0+tmp_h;
    bool ispush;
    bool done = false;
    while(!queue_pb.empty() && !done){
        // pop smallest
        curr_pb = queue_pb.top(); // NOTE that the operations below may modify front directly
        printBoard(&Brd, curr_pb.pb);
        // if this node is a goal state(m_cnt == 0) then return
        if(((curr_pb.pb >> 6) & static_cast<ULL>(15)) == 0){
            done = true;
            break;
        }
        // for each move
        for(int i=0; i < NumberOfDirection && !done; i++){    
            // if it is a valid move and not visited before
            next_pb.pb = Do_move(curr_pb.pb, static_cast<Direction>(i), NULL, ispush);
            if(next_pb.pb){
                // compute h
                tmp_h = compute_h(next_pb.pb);
                // if heuristic is infinity, then continue
                if(tmp_h == -1){
                    continue;
                }
                next_pb.g = curr_pb.g + 1;
                next_pb.f = next_pb.g + tmp_h;
                // if not visited before
                if(!visitedmap[next_pb.pb].visited){
                    // tag visited
                    visitedmap[next_pb.pb].visited = true;
                    visitedmap[next_pb.pb].f = next_pb.f;
                    // Save backtrack information
                    backtrackmap[next_pb.pb].dir = static_cast<Direction>(i);
                    backtrackmap[next_pb.pb].push = ispush;
                    // push into queue (this will be a copy)
                    queue_pb.push(next_pb);
                }
                // if have been visited before, check if the cost is smaller
                else{
                    // if this node is better than previous
                    assert(next_pb.f != -1);
                    if(next_pb.f < visitedmap[next_pb.pb].f){
                        // change f
                        visitedmap[next_pb.pb].f = next_pb.f;
                        // change backtrack information
                        backtrackmap[next_pb.pb].dir = static_cast<Direction>(i);
                        backtrackmap[next_pb.pb].push = ispush;
                    }
                }
            }
        }
        // pop out this element
        queue_pb.pop();
    }
    while(true){
        // 
        auto it = backtrackmap.find(curr_pb.pb);
        if(it == backtrackmap.end()){
            break;
        }
        // undo a move
        curr_pb.pb = Undo(curr_pb.pb, it->second);
        //
        History.push_back(it->second);
    }
    reverse(History.begin(), History.end());
    return true;

}


void bibfs(Vector_Move &History, const PlayerandBox &initial_pb, const vector<PlayerandBox> &goal_set){
    /*
        Bidirectional 
    */
    BacktrackMap forward_backtrackmap, backward_backtrackmap;
    VisitedMap forward_visitedmap, backward_visitedmap; 
    queue<PlayerandBox> forward_queue, backward_queue;
    PlayerandBox forward_parent_pb, forward_next_pb, backward_parent_pb, backward_next_pb, intersect_pb=0, tmp_pb=0;
    forward_backtrackmap.reserve(10000000), backward_backtrackmap.reserve(10000000);
    forward_visitedmap.reserve(10000000), backward_visitedmap.reserve(10000000);
    // put all things into queue
    // push 
    forward_queue.push(initial_pb);
    forward_visitedmap[initial_pb] = true;
    // push goal states into queue
    for(auto &x:goal_set){
        backward_queue.push(x);
        backward_visitedmap[x] = true;
    }
    // BidirBFS
    bool done = false;
    bool ispush = false;
    Move tmp_mv;
    while(!forward_queue.empty() && !backward_queue.empty() && !done){
        forward_parent_pb = forward_queue.front();
        forward_queue.pop();
        backward_parent_pb = backward_queue.front();
        backward_queue.pop();
        // expand forward
        for(int i = 0; i < NumberOfDirection; i++){
            forward_next_pb = Do_move(forward_parent_pb, static_cast<Direction>(i), NULL, ispush);
            // if not zero and not visited
            if(forward_next_pb && (!forward_visitedmap[forward_next_pb])){
                // tag visited
                forward_visitedmap[forward_next_pb] = true;
                // backtrack
                forward_backtrackmap[forward_next_pb].dir = static_cast<Direction>(i) ;
                forward_backtrackmap[forward_next_pb].push = ispush;
                // if goal
                if(((forward_next_pb >> 6) & static_cast<ULL>(15)) == 0){
                    intersect_pb = forward_next_pb;
                    done = true;
                    break;
                }
                // if intersect
                if(backward_visitedmap[forward_next_pb]){
                    intersect_pb = forward_next_pb;
                    done = true;
                    break;
                }
                // push queue
                forward_queue.push(forward_next_pb);
            }
        }
        // expand backward
        for(int i = 0; i < NumberOfDirection && !done; i++){
            for(int pull=0; pull < 2 && !done; pull++){
                backward_next_pb = reverse_Do_move(backward_parent_pb, static_cast<Direction>(i), pull, NULL);
                if(backward_next_pb && (!backward_visitedmap[backward_next_pb])){
                    // tag visited
                    backward_visitedmap[backward_next_pb] = true;
                    // 
                    backward_backtrackmap[backward_next_pb].dir = static_cast<Direction>(i);
                    backward_backtrackmap[backward_next_pb].push = pull;
                    // if reach initial pb or backward_pb is in forward visitedmap
                    if((backward_next_pb == initial_pb) || (forward_visitedmap[backward_next_pb])){
                        intersect_pb = backward_next_pb;
                        done = true;
                        break;
                    }
                    // push queue
                    backward_queue.push(backward_next_pb);
                }
            }
        }
    }
    // backtrack using intersect
    tmp_pb = intersect_pb;
    // forward
    BacktrackMap::iterator it;
    while(true){
        // 
        it = forward_backtrackmap.find(tmp_pb);
        if(it == forward_backtrackmap.end()){
            break;
        }
        // 
        tmp_pb = Undo(tmp_pb, it->second);
        //
        History.push_back(it->second);
    }
    reverse(History.begin(), History.end());
    tmp_pb = intersect_pb;
    // backward
    while(true){
        it = backward_backtrackmap.find(tmp_pb);
        if(it == backward_backtrackmap.end()){
            break;
        }
        tmp_pb = reverse_Undo(tmp_pb, it->second);
        History.push_back(it->second);
    }
}

void reverse_bfs(Vector_Move &History, const PlayerandBox &initial_pb, const vector<PlayerandBox> &goal_set){
    BacktrackMap backtrackmap;
    VisitedMap visitedmap;
    PlayerandBox curr_pb, next_pb;
    queue<PlayerandBox> queue_pb;
    // push goal states into queue
    for(auto &x:goal_set){
        queue_pb.push(x);
        visitedmap[x] = true;
    }
    bool done = false;
    Move tmp_mv;
    while(!queue_pb.empty() && !done){
        curr_pb = queue_pb.front();
        // TODO optimize bfs (tag visited sooner)
        for(int i=0; i < NumberOfDirection && !done; i++){
            for(int pull=0; pull < 2 && !done; pull++){
                next_pb = reverse_Do_move(curr_pb, static_cast<Direction>(i), pull, NULL);
                if(next_pb){
                    if(!visitedmap[next_pb]){
                        // tag visited
                        visitedmap[next_pb] = true;
                        // Save backtrack information
                        backtrackmap[next_pb].dir = static_cast<Direction>(i);
                        backtrackmap[next_pb].push = pull;
                        // if we find initial_pb
                        if(next_pb == initial_pb){
                            done = true;
                            break;
                        }
                        queue_pb.push(next_pb);
                    }
                }
            }
        }
        queue_pb.pop();
    }
    while(true){
        // 
        auto it = backtrackmap.find(next_pb);
        if(it == backtrackmap.end()){
            break;
        }
        // undo a move
        next_pb = reverse_Undo(next_pb, it->second);
        //
        History.push_back(it->second);
    }
}

bool bfs(Vector_Move &History, const PlayerandBox &initial_pb){
    /*  
        Input: 
            Brd: a clear board
            History: 
            initial player and boxes
    */
    //
    BacktrackMap backtrackmap; backtrackmap.reserve(10000000);
    VisitedMap visitedmap; visitedmap.reserve(10000000);
    PlayerandBoxSet depth_limit_board_sets; //depth_limit_board_sets.reserve(10000000);
    PlayerandBox curr_pb, next_pb;
    // init queue
    queue<PlayerandBox> queue_pb;
    // insert start node into queue
    queue_pb.push(initial_pb);
    // tag visited
    visitedmap[initial_pb] = true;
    bool ispush;
    bool done = false;
    Move tmp_mv;
    while(!queue_pb.empty() && !done){
        // pop queue(put information into curr_brd)
        curr_pb = queue_pb.front(); // NOTE that the operations below may modify front directly
        // for each move
        for(int i=0; i < NumberOfDirection && !done; i++){    
            // if it is a valid move and not visited before
            next_pb = Do_move(curr_pb, static_cast<Direction>(i), NULL, ispush);
            if(next_pb){
                // if not visited before
                if(!visitedmap[next_pb]){
                    // tag visited
                    visitedmap[next_pb] = true;
                    // a move
                    tmp_mv = Move(static_cast<Direction>(i), ispush);
                    // Save backtrack information
                    backtrackmap[next_pb] = tmp_mv;
                    // if this node is a goal state then end
                    if(((next_pb >> 6) & static_cast<ULL>(15)) == 0){
                        // printf("Bitset:\n");
                        // cout << bitset<64>(static_cast<ULL>(15)) << "\n";
                        // cout << bitset<64>((next_pb >> 6)) << "\n";
                        // cout << bitset<64>((next_pb >> 6) & static_cast<ULL>(15)) << "\n";
                        done = true;
                        break;
                    }
                    // push into queue (this will be a copy)
                    queue_pb.push(next_pb);
                }
            }
        }
        // pop out this element
        queue_pb.pop();
    }
    if(!done){
        return false;
    }
    while(true){
        // 
        auto it = backtrackmap.find(next_pb);
        if(it == backtrackmap.end()){
            break;
        }
        // undo a move
        next_pb = Undo(next_pb, it->second);
        //
        History.push_back(it->second);
    }
    // reverse history
    reverse(History.begin(), History.end());
    return true;
}

// ===========================

bool Get_Board(FILE *const fin, PlayerandBox &initial_pb){
    // init all
    Brd.init();
    initial_pb = 0;
    //
    if(fscanf(fin, "%d%d ", &Brd.height, &Brd.width) != 2){
        return false;
    }
    //
    int curr_number = 0;
    box_number = 0;
    memset(edgeGoalNumber, 0, sizeof(edgeGoalNumber));
    for(int i=0; i<=Brd.height-1; ++i){
        if(!fgets(Brd.board[i], WIDTH_MAX+2, fin)){
            assert(0);
        }
        Brd.board[i][Brd.width] = 0;
        for(int j=0; j<=Brd.width-1; ++j){
            char *curr = &Brd.board[i][j];
            curr_number = i*Brd.width + j + OFFSET;
            // record edgeGoalNumber
            if(i == 0){
                edgeGoalNumber[0]++;
            }
            if(i == (Brd.height-1)){
                edgeGoalNumber[1]++;
            }
            if(j == 0){
                edgeGoalNumber[2]++;
            }
            if(j == (Brd.width-1)){
                edgeGoalNumber[3]++;
            }
            // record player position
            if(*curr == '@' || *curr == '+'){
                initial_pb |= static_cast<ULL>(i*Brd.width + j);
            }

            // record box position
            if(*curr == '$' || *curr == '*'){
                box_number++;
                initial_pb |= static_cast<ULL>(1) << (curr_number);
                // m_cnt
                if(*curr == '$'){
                    initial_pb += static_cast<ULL>(1) << 6;
                }
            }
            // record wall position
            if(*curr == '#'){
                Brd.wallboard |= static_cast<ULL>(1) << (curr_number);
            }
            // record goal position
            if(*curr == '+' || *curr == '*' || *curr == '.'){
                Brd.goalboard |= static_cast<ULL>(1) << (curr_number);
            }
        }
    }
    return true;
}


bool Inside(int const i, int const j){
    // check if a position is in board and not in wall
    return 0<=i && i<=Brd.height-1 && 0<=j && j<=Brd.width-1 
        && ((Brd.wallboard & (static_cast<ULL>(1) << (i * Brd.width + j + OFFSET) )) == 0);
    // if this bit operation result in zero, means no wall at i, j
}


PlayerandBox Do_move(const PlayerandBox &curr_pb, Direction const dir, Vector_Move *const history, bool &ispush){
    PlayerandBox new_pb = curr_pb;
    ispush = false;
    // if there is no space to move
    // 63 is 111111
    int p_row = (new_pb & static_cast<ULL>(63)) / Brd.width;
    int p_col = (new_pb & static_cast<ULL>(63)) % Brd.width;
    if(!Inside(p_row+Di[dir], p_col+Dj[dir])){
        return 0;
    }
    int next_number = (p_row+Di[dir]) *(Brd.width) + (p_col+Dj[dir]);
    int nnext_number = (p_row+2*Di[dir]) *(Brd.width) + (p_col+2*Dj[dir]);
    ULL next_bit = static_cast<ULL>(1) << (next_number + OFFSET);
    ULL nnext_bit = static_cast<ULL>(1) << (nnext_number + OFFSET);
    // find if there is a box
    if((new_pb & next_bit) != 0){
        // check if nnext is legal and not out of range
        if(!Inside(p_row+2*Di[dir], p_col+2*Dj[dir])){
            return 0;
        }else{
            // if there is a box at nnext, then it is not a legal move
            if((new_pb & nnext_bit) != 0 ){
                return 0;
            }else{
                // NEW: check if a dead push
                if(check_dead_push(new_pb, dir)){
                    // printf("Dir %d\n", dir);
                    // printBoard(&Brd, curr_pb);
                    return 0;
                }
                // modify m_cnt
                if((Brd.goalboard & next_bit) != 0){
                    // if next is goal(& is not zero) but nnext is not goal(& is zero)
                    if((Brd.goalboard & nnext_bit) == 0){
                        // m_cnt add 1
                        new_pb += static_cast<ULL>(1) << 6;
                    }
                }else{
                    // if next is not goal but nnext is goal(& is not zero)
                    if((Brd.goalboard & nnext_bit) != 0){
                        new_pb -= static_cast<ULL>(1) << 6;
                    }
                }
                // move box from next to nnext
                // mask next box out
                new_pb &= ~(next_bit);
                // put nnext box on
                new_pb |= nnext_bit;
                // set up push
                ispush = true;
            }

        }
    }else{
        ispush = false;
    }
    // move player
    p_row += Di[dir], p_col += Dj[dir];
    // mask original player position (avoid adding negative number)
    new_pb &= (~static_cast<ULL>(63));
    // put player position onto it
    new_pb |= (static_cast<ULL>(p_row * Brd.width + p_col));
    // append to history vector
    if(history){
        history->push_back(Move(dir, ispush));
    }
    return new_pb;
}

PlayerandBox reverse_Do_move(const PlayerandBox &curr_pb, Direction const dir, bool pull, Vector_Move *const reverse_history){
    PlayerandBox new_pb = curr_pb;
    int p_row = (new_pb & static_cast<ULL>(63)) / Brd.width;
    int p_col = (new_pb & static_cast<ULL>(63)) % Brd.width;
    int curr_number = p_row * Brd.width + p_col;
    ULL curr_bit = static_cast<ULL>(1) << (curr_number + OFFSET);
    // check if prev is a valid square
    if(!Inside(p_row-Di[dir], p_col-Dj[dir])){
        return 0;
    }
    // if prev is a box, then return false
    if((new_pb & curr_bit) != 0){
        return 0;
    }
    // 
    if(pull){
        // check if next position is valid
        if(!Inside(p_row+Di[dir], p_col+Dj[dir])){
            return 0;
        }
        int next_number = (p_row+Di[dir]) * Brd.width + (p_col+Dj[dir]);
        ULL next_bit = static_cast<ULL>(1) << (next_number + OFFSET);
        // if there is no box, return
        if((new_pb & next_bit) == 0){
            return 0;
        }
        // compute m_cnt
        if((Brd.goalboard & next_bit) != 0){
            // if move from goal to nongoal
            if((Brd.goalboard & curr_bit) == 0){
                new_pb += static_cast<ULL>(1) << 6;
            }
        }else{
            // if move from nongoal to goal
            if((Brd.goalboard & curr_bit) != 0){
                new_pb -= static_cast<ULL>(1) << 6;
            }
        }
        // erase next_number box
        new_pb &= ~(next_bit);
        // put box on curr_number
        new_pb |= curr_bit;
    }
    // move player
    p_row -= Di[dir], p_col -= Dj[dir];
    new_pb &= ~(static_cast<ULL>(63));
    new_pb |= (static_cast<ULL>(p_row * Brd.width + p_col));
    // record
    if(reverse_history){
        reverse_history->push_back(Move(dir, pull)); // if there is a pull, that means a push in forward
    }
    return new_pb;
}


PlayerandBox Undo(const PlayerandBox &curr_pb, Move const mv){
    PlayerandBox new_pb = curr_pb;
    // if there is no space to move
    // 63 is 111111
    int p_row = (new_pb & static_cast<ULL>(63)) / Brd.width;
    int p_col = (new_pb & static_cast<ULL>(63)) % Brd.width;
    int curr_number = p_row * Brd.width + p_col;
    // assert prev is Inside
    assert(Inside(p_row-Di[mv.dir], p_col-Dj[mv.dir]));
    if(mv.push){
        int next_number = (p_row+Di[mv.dir])*Brd.width + (p_col+Dj[mv.dir]);
        ULL next_bit = static_cast<ULL>(1) << (next_number + OFFSET);
        ULL curr_bit = static_cast<ULL>(1) << (curr_number + OFFSET);
        // assert there is a square
        assert(Inside(p_row+Di[mv.dir], p_col+Dj[mv.dir]));
        // assert there is a box
        assert((new_pb & next_bit) != 0);
        // pull box out
        // mask next box out
        new_pb &= ~next_bit;
        // put box at curr_number
        new_pb |= curr_bit;
        // change m_cnt
        if((Brd.goalboard & next_bit) != 0){
            // if next_number is a goal(not equal to 0) but curr_number is not a goal(equal to 0)
            if((Brd.goalboard & curr_bit) == 0){
                // m_cnt add 1
                new_pb += static_cast<ULL>(1) << 6;
            }
        }else{
            // if next_number is not a goal but curr_number is a goal
            if((Brd.goalboard & curr_bit) != 0){
                // m_cnt minus 1
                new_pb -= static_cast<ULL>(1) << 6;
            }
        }
    }
    // move player
    p_row -= Di[mv.dir], p_col -= Dj[mv.dir];
    // mask original player position (avoid adding negative number)
    new_pb &= (~static_cast<ULL>(63));
    // put player position onto it
    new_pb |= (static_cast<ULL>(p_row * Brd.width + p_col));
    return new_pb;
}
PlayerandBox reverse_Undo(const PlayerandBox &curr_pb, Move const mv){
    // Do a move
    bool ispush;
    return Do_move(curr_pb, mv.dir, NULL, ispush);
}

void generate_Goal_states(vector<PlayerandBox> &goal_set){
    PlayerandBox goal_state_pb = 0;
    for(int i=0; i < (Brd.width * Brd.height); i++){
        // if there is a goal to place box
        if((Brd.goalboard & (static_cast<ULL>(1) << (i+OFFSET))) != 0){
            // put box on goal place
            goal_state_pb |= static_cast<ULL>(1) << (i+OFFSET);
        }
    }
    for(int i=0; i < (Brd.width * Brd.height); i++){
        // if there is no wall here
        if((Brd.wallboard & (static_cast<ULL>(1) << (i+OFFSET))) == 0){
            // if it is not a goal(because already have box)
            if((Brd.goalboard & (static_cast<ULL>(1) << (i+OFFSET))) == 0){
                // place player
                goal_state_pb |= static_cast<ULL>(i);
                // push back
                goal_set.push_back(goal_state_pb);
                // pop back
                goal_state_pb &= ~static_cast<ULL>(63);

            }
        }
    }

}
// check if this push can cause problem
bool check_dead_push(const PlayerandBox &curr_pb, const Direction &dir){
    int p_row = (curr_pb & static_cast<ULL>(63)) / Brd.width;
    int p_col = (curr_pb & static_cast<ULL>(63)) % Brd.width;
    int tmp_row = -1, tmp_col = -1;
    int next_row = p_row+2*Di[dir], next_col = p_col+2*Dj[dir];
    // nnnext_bit (this is sure to be Inside (because in Do_move we already check!!))
    ULL nnext_bit = static_cast<ULL>(1) << ((p_row+2*Di[dir])*Brd.width + p_col+2*Dj[dir] + OFFSET);
    // if it is a goal state then return false!!!!
    if((Brd.goalboard & nnext_bit) > 0){
        return false;
    }
    
    // if nnnext is a wall or out of range
    if((!Inside(p_row+3*Di[dir], p_col+3*Dj[dir])) ){
        // check if nnext position's remain 2 directions have at least 1 (wall or box or out of range)
        for(int d=0; d < NumberOfDirection; d++){
            // I can skip this one
            if(d == dir){
                continue;
            }
            tmp_row = p_row+2*Di[dir]+Di[d], tmp_col = p_col+2*Dj[dir]+Dj[d];
            // if out of range or wall
            if(!Inside(tmp_row, tmp_col)){
                return true;
            }
        }
        // if the box will be move to edge
        tmp_row = p_row+3*Di[dir], tmp_col = p_col+3*Dj[dir];
        if(!(0<=tmp_row && tmp_row<=Brd.height-1 && 0<=tmp_col && tmp_col<=Brd.width-1)){
            counter = 0;
            if(next_row == 0){
                // check edge 0
                for(int i=0; i<Brd.width; i++){
                    if(counter > edgeGoalNumber[0]){
                        return true;
                    }
                    if((curr_pb & (static_cast<ULL>(1) << (i + OFFSET))) > 0){
                        counter++;
                    }
                }
            }
            counter = 0;
            if(next_row == Brd.height-1){
                // check edge 1
                for(int i=Brd.width*(Brd.height-1); i<Brd.width*Brd.height; i++){
                    if(counter > edgeGoalNumber[1]){
                        return true;
                    }
                    if((curr_pb & (static_cast<ULL>(1) << (i + OFFSET))) > 0){
                        counter++;
                    }
                }
            }
            counter = 0;
            if(next_col == 0){
                // check edge 2
                for(int i=0; i<Brd.width*Brd.height; i+=Brd.width){
                    if(counter > edgeGoalNumber[2]){
                        return true;
                    }
                    if((curr_pb & (static_cast<ULL>(1) << (i + OFFSET))) > 0){
                        counter++;
                    }
                }
            }
            counter = 0;
            if(next_col == Brd.height-1){
                // check edge 3
                for(int i=Brd.width-1; i<Brd.width*Brd.height; i+=Brd.width){
                    if(counter > edgeGoalNumber[3]){
                        return true;
                    }
                    if((curr_pb & (static_cast<ULL>(1) << (i + OFFSET))) > 0){
                        counter++;
                    }
                }
            }

        }
    }
    return false;
}


// =========================== DEBUG ==============================
void printBoard(Board *const brd, const PlayerandBox &curr_pb){
    Board new_brd = *brd;
    //
    int m_cnt = (curr_pb >> 6) & static_cast<ULL>(15);
    int p_row = (curr_pb & static_cast<ULL>(63)) / new_brd.width;
    int p_col = (curr_pb & static_cast<ULL>(63)) % new_brd.width;
    // meta info
    fprintf(stderr, "===========================================\n");
    fprintf(stderr, "height: %d, width %d\n", new_brd.height, new_brd.width);
    fprintf(stderr, "m_cnt: %d, p_row: %d, p_col: %d\n", m_cnt, p_row, p_col);
    std::cout << bitset<64>(curr_pb) << "\n"; 
    //
    for(int row=0; row<new_brd.height; row++){
        for(int col=0; col<new_brd.width;col++){
            char *curr = &new_brd.board[row][col];
            int curr_number = row * new_brd.width + col;
            // check if this is a wall
            if((new_brd.wallboard & (static_cast<ULL>(1) << (curr_number + OFFSET))) != 0){
                *curr = '#';
            }else{
                // check if this is a goal
                bool goal = false;
                if((new_brd.goalboard & (static_cast<ULL>(1) << (curr_number + OFFSET))) != 0){
                    goal = true;
                }
                // check if player
                if(row == p_row && col == p_col){
                    if(goal){
                        *curr = '+';
                    }else{
                        *curr = '@';
                    }
                }else{
                    // check if a box 
                    if((curr_pb & (static_cast<ULL>(1) << (curr_number + OFFSET))) != 0){
                        if(goal){
                            *curr = '*';
                        }else{
                            *curr = '$';
                        }
                    }else{
                        if(goal){
                            *curr = '.';
                        }else{
                            *curr = '-';
                        }
                    }
                }
            }
            // put char
            fprintf(stderr, "%c", *curr);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "===========================================\n");
}
