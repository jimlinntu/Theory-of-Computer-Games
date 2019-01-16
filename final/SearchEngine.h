#ifndef SE
#define SE
#include"anqi.hh"
#include<chrono>
#include<random>
#include<limits>
#include<fstream>
#include<vector>
#include<bitset>
#include<assert.h>
#include<time.h>
#include<iostream>
#include<sys/time.h>
#include"TranspositionTable.hh"
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

using namespace std;
using SCORE = double;

static const POS ADJ[32][4]={
	{ 1,-1,-1, 4},{ 2,-1, 0, 5},{ 3,-1, 1, 6},{-1,-1, 2, 7},
	{ 5, 0,-1, 8},{ 6, 1, 4, 9},{ 7, 2, 5,10},{-1, 3, 6,11},
	{ 9, 4,-1,12},{10, 5, 8,13},{11, 6, 9,14},{-1, 7,10,15},
	{13, 8,-1,16},{14, 9,12,17},{15,10,13,18},{-1,11,14,19},
	{17,12,-1,20},{18,13,16,21},{19,14,17,22},{-1,15,18,23},
	{21,16,-1,24},{22,17,20,25},{23,18,21,26},{-1,19,22,27},
	{25,20,-1,28},{26,21,24,29},{27,22,25,30},{-1,23,26,31},
	{29,24,-1,-1},{30,25,28,-1},{31,26,29,-1},{-1,27,30,-1}
};

class SearchEngine{
public:
	// Random number
	static mt19937_64 gen;
	static SCORE INF;
	static SCORE NINF;
	static SCORE vMax;
	static SCORE vMin;
	static SCORE finScore[7]; // 0: 帥將, ..., 6: 兵卒
	int cutOffDepth = 7; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start, stop;
	TranspositionTable transTable[2]; // 0: myself, 1: opponent 
	TranspositionTable visitedTable; // 檢查有沒有三循環的 table
	fstream logger;

	SearchEngine(){
		this->logger.open("log.txt",  ios::out | ios::trunc);
		this->logger << "Start Search" << "\n";
	}
	~SearchEngine(){
		this->logger.close();
	}

	// 做出決定
	MOV Play(const BOARD &B, int remain_milliseconds){
		MOV BestMove(-1, -1);
		// 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// 如果之前參訪過
			*record.flag += 1; // add visit time
			cerr << "[*] Visited Board has: " << bitset<64>(B.hashKey) << "\n";
		}else{
			this->visitedTable.insert(B.hashKey, -1, 1, -1);
		}

		// TODO: Add starting board heuristic play
		if(this->specialBoardCondition(B, BestMove)){
			return BestMove;
		}
		// Pre-evaluation
		SCORE preScore = this->Eval(B);
		this->logger << "Prescore is: " << preScore << "\n" << flush;
		// NegaMax 
		// * 越多暗子的時候花時間少一點
		// this->timeOut = remain_milliseconds * ((33 - B.sumCnt) / 32) - 1;
		this->timeOut = 5000;
		
		gettimeofday(&this->start, NULL);
		cerr << "[*] Start negaScout" << "\n";
		SCORE negaScore= this->negaScout(B, SearchEngine::vMin, SearchEngine::vMax, BestMove, 0);
		cerr << "Negascore: " << negaScore << "\n";
		
		// 如果 negaScout 搜不出步來
		if(BestMove.st == -1 || BestMove.ed == -1){
			assert(randomFlip(B, BestMove));
		}
		cerr << "[*] End negaScout" << "\n";
		
		if(BestMove.st == BestMove.ed){
			// 翻棋子不會造成 3 循環(因為翻出來之後一定沒辦法往回走)
			// 所以不需要紀錄曾經參訪, 因為一定不會重複盤面
		}
		else{
			cerr << "[*] Checking Repetition......\n";
			// 如果不是翻棋，要考慮看看有沒有可能造成 3 循環
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15));
			cerr << "[*] nextB.hashKey: " << bitset<64>(nextB.hashKey) << "\n";
			record = this->visitedTable.getVal(nextB.hashKey);
			// 如果 flag 已經是 2 了, 那就亂走
			if(record.flag != nullptr) this->logger << "[*] This Board has been visited: " << *record.flag << "\n";
			if(record.flag != nullptr && (*record.flag) >= 2){
				if(B.sumCnt > 0) {
					// 翻子絕對不會造成三循環
					this->randomFlip(B, BestMove);
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove);
					assert(BestMove.st != BestMove.ed);
					
				}	
			}			
			// 如果最後選出來是走吃步, 就記一下參訪過
			if(BestMove.st != BestMove.ed){
				nextB = B;
				nextB.DoMove(BestMove, FIN(15));
				record = this->visitedTable.getVal(nextB.hashKey);
				if(record.flag != nullptr){
					*record.flag += 1;
				}else{
					this->visitedTable.insert(nextB.hashKey, -1, 1, -1);
				}
			}
			
		}
		

		return BestMove;
	}

	bool specialBoardCondition(const BOARD &B, MOV &BestMove){
		CLR oppColor = B.who^1;
		// * who 還沒決定時
		if(B.who == -1) {
			assert(randomFlip(B, BestMove));
			return true;
		}
		// * 全部�m還沒翻時
		if(B.sumCnt == 32){
			// 這步一定會改變 BestMove
			assert(randomFlip(B, BestMove));
			return true;
		}
		// * 如果對方砲已經出現, 而且
		
		// * 如果帥已經出現, 而且我的兵卒�m還有五個的時候
		
		// * 檯面上目前�m是別人的子的時候
		// * 檯面上目前�m還是自己的子的時候
		return false;
	}
	
	bool randomFlip(const BOARD &B, MOV &BestMove){
		// * 不要翻到別人的子旁邊
		// * 如果敵人翻到砲或帥, 直接翻他旁邊幹掉他
		if(B.sumCnt==0) return false;
		vector<MOV> movList;
		CLR oppColor = (B.who^1);
		
		for(POS p = 0; p < 32; p++){
			// 如果這個子是暗子
			bool isDangerous = false;
			if(B.fin[p] == FIN_X){
				//檢查四面八方有沒有敵人的子
				for(int dir=0; dir < 4; dir++){
					POS q = ADJ[p][dir];
					// 如果是敵方的子, 盡量不要翻旁邊(除了砲或帥)
					if(GetColor(B.fin[q]) == oppColor){
						isDangerous = true;
						// 如果是砲 或 帥, �m直接翻他隔壁
						if(GetLevel(B.fin[q]) == LVL_C && GetLevel(B.fin[q]) == LVL_K){
							BestMove = MOV(p, p);
							return true;
						}
					}
				}
				// 如果不危險才 push back
				if(!isDangerous) movList.push_back(MOV(p, p));
			}
		}
		uniform_int_distribution<int> U(0, movList.size()-1);
		BestMove = movList[U(gen)];
		return true;
	}
	void randomMove(const BOARD &B, MOV &BestMove, const MOV &prohitbitMove){
		vector<MOV> movList; // population
		for(POS p = 0; p < 32; p++){
			FIN pf = B.fin[p];
			// 自己的棋才繼續
			if(GetColor(pf) != B.who) continue;

			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // 如果是不合法的步就跳過
				FIN qf = B.fin[q];
				// 是可以走吃的步  且 不可以是被禁止的步
				if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
					movList.push_back(MOV(p, q));
					// (ChkEats 會檢查是否是空或對方子)如果對方是王, 幹掉再說
					if(GetColor(qf) == (B.who^1) && GetLevel(qf) == LVL_K){
						BestMove = MOV(p, q);
						return;
					}
				}
			}
		}
		// random sample one move
		uniform_int_distribution<int> U(0, movList.size()-1);
		BestMove = movList[U(gen)];
	}

	// 仔細思考
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		MOVLST lst;
		// 只在 root 的時候 或 小於 3 子沒翻出來的時候 才模擬翻出子來的情況, 其他情況�m只對翻開來的子做搜尋
		// if(depth == 0 || B.sumCnt < 3){
		// 	B.MoveGenWithFlip(lst);
		// }else{
			B.MoveGen(lst);
		// }
		// * Time out
		// * If win (winner is not other)
		// * depth cut off
		// * no move can take
		gettimeofday(&this->stop, NULL);
		const int milliElapsed = (stop.tv_sec - start.tv_sec) * 1000 + (stop.tv_usec-start.tv_usec) / 1000;
		if(milliElapsed > this->timeOut || B.getWinner() != -1 
			|| depth == this->cutOffDepth || lst.num == 0){
			this->logger << "[*] Scout Depth: " << depth << "\n";
			this->logger << "[*] Board: " << B.who << "\n";
			this->logger << "[*] Elapse seconds: " << milliElapsed / 1000 << "\n";
			return Eval(B);
		}
		SCORE m = SearchEngine::NINF, n = beta;
		SCORE t;
		BOARD nextB;
		for(int i = 0; i < lst.num; i++){
			nextB = B;
			// * Chance node
			// TODO:
			if(lst.mov[i].st == lst.mov[i].ed){
				// Chance Search()
				t = this->chanceSearch(B, lst.mov[i], MAX(alpha, m), beta, BestMove, depth);
			}
			// * Deterministic node
			else{
				nextB.DoMove(lst.mov[i], FIN_E); // 後面 FIN_E 用不到
				// Scout
				t = -this->negaScout(nextB, -n, -MAX(alpha, m), BestMove, depth+1);
			}
			// * Take max
			if(t > m){
				if(n == beta || (this->cutOffDepth - depth) < 3 || t >= beta){
					m = t;
				}else{
					// Re-search
					m = -negaScout(nextB, -beta, -t, BestMove, depth+1);
				}
				// record BestMove
				if(depth == 0){
					BestMove = lst.mov[i];
				}
				
				if(m >= beta){
					break; //return m
				}
			}
			n = MAX(alpha, m) + 1; // null window
		}

		return m;
	}	
	// WARNING: vMin vMax must symmetric
	SCORE chanceSearch(const BOARD &B, const MOV &mv, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		// 還有沒有翻出來的棋
		assert(B.sumCnt > 0);
		assert(mv.st == mv.ed);
		// 
		double A, B_;
		double m=SearchEngine::vMin, M = SearchEngine::vMax;
		double t;
		double tmpLocalRatio;
		double weightVSum = 0;
		bool isfirst = true;
		BOARD nextB;
		// 
		for(int i = 0; i < 14; i++){
			// 如果那個棋子沒有了
			if(B.cnt[i] == 0){
				continue;
			}
			// 如果這個兵種還有存在的話
			else{
				nextB = B;
				// 假設這個兵種被翻開來
				nextB.DoMove(mv, FIN(i));
				// 
				if(!isfirst){
					// (c / w1) * (alpha - vMax)
					tmpLocalRatio = ((double)B.sumCnt / (double)B.cnt[i]);
					A = tmpLocalRatio * (alpha - SearchEngine::vMax) + SearchEngine::vMax;
					B_ = tmpLocalRatio * (beta - SearchEngine::vMin) + SearchEngine::vMin;
					isfirst = false;
				}
				// TODO: 要加負號嗎？ 我覺得應該要加在這裡, 因為 [alpha, beta] 現在是針對這個 node 的
				t = -this->negaScout(nextB, -MIN(B_, SearchEngine::vMax), -MAX(A, SearchEngine::vMin), BestMove, depth+1);
				// update
				tmpLocalRatio = ((double)B.cnt[i] / (double)B.sumCnt);
				m = m + tmpLocalRatio * (t - SearchEngine::vMin);
				M = M + tmpLocalRatio * (t - SearchEngine::vMax);
				// Cut off
				if(t >= B_) return m;
				if(t <= A) return M;
				// Weighted value(w * v)
				weightVSum += (double)B.cnt[i] * t;
				// Update
				A = A + SearchEngine::vMax - t;
				B_ = B_ + SearchEngine::vMin - t;
			}
		}
		// vsum / c
		return weightVSum / (double)B.sumCnt;
	}

	// h(B): Nega version(return h(p) )
	SCORE Eval(const BOARD &B) {
		assert(B.who != -1);
		SCORE cnt[2]={0,0}; // 0: 紅方, 1: 黑方
		// 計算翻開棋的子力分數
		for(POS p=0;p<32;p++){
			const CLR c=GetColor(B.fin[p]);
			// 如果不是未翻開的
			if(c!=-1){
				// 計算子力分數
				cnt[c] += SearchEngine::finScore[GetLevel(B.fin[p])];
			}
		}
		// 計算未翻開的子力分數
		for(int i=0;i<14;i++){
			cnt[i / 7] += (double)B.cnt[i] * SearchEngine::finScore[i % 7];
		}
		// MIN node: 越高代表對 max node 越不利
		// Max node: 越高代表對 min node 越不利
		// TODO: 距離也要算一點分數！！！
		return cnt[B.who]-cnt[B.who^1];
	}

	static SCORE sumOfFinScore(){
		static const int tbl[]={1,2,2,2,2,2,5};
		SCORE sum = 0;
		for(int i = 0; i < 7; i++){
			sum += (double)tbl[i] * SearchEngine::finScore[i];
		}
		assert((int)sum == 19357);
		return sum;
	}
};

mt19937_64 SearchEngine::gen(random_device{}());
// 子力分數(0: 帥, ..., 6:卒)
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200};
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore();
SCORE SearchEngine::vMin = -SearchEngine::vMin;
#endif