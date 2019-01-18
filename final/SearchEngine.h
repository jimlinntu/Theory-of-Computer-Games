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
	timeval start;
	TranspositionTable transTable[2]; // 0: red, 1: black
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
		// Retrieve from visitedTable 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// 如果之前參訪過
			*record.flag += 1; // add visit time
		}else{
			this->visitedTable.insert(B.hashKey, -1, 1, -1);
		}
		// 開局 Rule
		if(this->specialBoardCondition(B, BestMove)){
			return BestMove;
		}
		assert(B.who != -1); // B.who == -1 會被 special condition 擋掉
		// Pre-evaluation
		SCORE preScore = this->Eval(B);
		this->logger << "Prescore is: " << preScore << "\n" << flush;
		// NegaMax 
		// * 越多暗子的時候花時間少一點
		this->timeOut = MIN(10000 , remain_milliseconds / 4);
		
		gettimeofday(&this->start, NULL);
		cerr << "[*] Start negaScout" << "\n";
		// ID NegaScout
		SCORE negaScore;
		// 
		negaScore = this->negaScout(B, SearchEngine::vMin, SearchEngine::vMax, BestMove, 0);
		cerr << "PreScore: " << preScore << "\n";
		cerr << "Negascore: " << negaScore << "\n";
		cerr << "[*] End negaScout" << "\n";
		
		// 如果因為沒有 MOVLIST, 而沒有步的話, 亂翻
		if(BestMove.st == -1 || BestMove.ed == -1){
			// 如果沒有移動步的話一定是有子可以翻 (除非卡死 ex. 兩隻砲卡死兵, 但是這個會先被 Judge 判定結束)
			assert(randomFlip(B, BestMove));
		}
		
		if(BestMove.st == BestMove.ed){
			// 翻棋子不會造成 3 循環(因為翻出來之後一定沒辦法往回走)
			// 所以不需要紀錄曾經參訪, 因為一定不會重複盤面
		}
		// 如果是走子就要檢查會不會重複
		else{
			cerr << "[*] Checking Repetition......\n";
			// FIXME: 會卡住？？
			// 如果不是翻棋，要考慮看看有沒有可能造成 3 循環
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15)); // FIN(15) 不會用到
			record = this->visitedTable.getVal(nextB.hashKey);
			// 如果 flag 已經是 2 了, 就不要再走這一步, 改成亂翻或是亂走
			if(record.flag != nullptr && (*record.flag) >= 1){
				cerr << "[*] visit count: " << *record.flag << "\n";
				// 如果還有子可以翻
				if(B.sumCnt > 0) {
					// 翻子絕對不會造成三循環
					assert(this->randomFlip(B, BestMove)); // 一定會可以翻
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove); // 亂走, 但不能走到 prohibit move
					assert(BestMove.st != BestMove.ed);
					
				}
			}

			// (BestMove 有可能已經換了)如果最後選出來是走吃步, 就記住下一步參訪次數, 如果是翻子步的話就不用記了，因為翻子不會造成循環
			if(BestMove.st != BestMove.ed){
				nextB = B;
				nextB.DoMove(BestMove, FIN(15));
				record = this->visitedTable.getVal(nextB.hashKey);
				if(record.flag != nullptr){
					*record.flag += 1;
					assert(*record.flag < 32767); // 應該不能 overflow
				}else{
					this->visitedTable.insert(nextB.hashKey, -1, 1, -1);
				}
			}
		}
		assert(BestMove.st != -1 && BestMove.ed != -1);
		cerr << "[*] BestMove I choose: " << BestMove.st << " " << BestMove.ed << "\n";
		return BestMove;
	}

	bool specialBoardCondition(const BOARD &B, MOV &BestMove){
		// * who 還沒決定時 || B.sumCnt == 32
		if(B.who == -1 || B.sumCnt == 32) {
			assert(randomFlip(B, BestMove)); // 隨便翻
			return true;
		}
		// * 如果對方砲或王已經出現時, 就直接翻他旁邊(要夠多暗子時才做這件事)
		assert(B.who != -1);
		const CLR oppColor = B.who^1;
		if((B.sumCnt > 28) && (B.cnt[FIN_C + oppColor*7] < 2 || B.cnt[FIN_K + oppColor*7] == 0)){
			assert(randomFlip(B, BestMove));
			return true;
		}
		
		// * 檯面上目前�m是別人的子的時候
		// * 檯面上目前�m還是自己的子的時候
		return false;
	}
	
	bool randomFlip(const BOARD &B, MOV &BestMove){
		// * 不要翻到別人的子旁邊
		// * 除了如果敵人翻到砲或帥, 直接翻他旁邊幹掉他
		if(B.sumCnt==0) return false;
		vector<MOV> movList;
		vector<MOV> dangerList;
		CLR oppColor = (B.who == -1)? (-1):(B.who^1) ; // 
		// 掃過所有 position
		for(POS p = 0; p < 32; p++){
			bool isDangerous = false;
			// 如果這個子不是暗子的話就跳過
			if(B.fin[p] != FIN_X) continue;
			// 檢查這個子的四面八方有沒有敵人的子
			for(int dir=0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				// 如果不是合理的步就跳過
				if(q == -1) continue;
				// 如果是敵方的子, 盡量不要翻旁邊(除了砲或帥), 如果是 unknown 顏色代表�m可以翻
				if((oppColor != -1) && (GetColor(B.fin[q]) == oppColor)){
					isDangerous = true;
					// 如果是砲 或 帥, �m直接翻他隔壁, 賭可以把它迅速吃掉
					if((GetLevel(B.fin[q]) == LVL_C) || (GetLevel(B.fin[q]) == LVL_K)){
						BestMove = MOV(p, p);
						return true;
					}
				}
			}
			// 如果不危險才 push back
			if(!isDangerous) movList.push_back(MOV(p, p));
			else dangerList.push_back(MOV(p, p));
		}
		
		if(movList.size() == 0){
			assert(dangerList.size() > 0);
			uniform_int_distribution<int> U(0, dangerList.size()-1);
			BestMove = dangerList[U(gen)];
		}else{
			uniform_int_distribution<int> U(0, movList.size()-1);
			BestMove = movList[U(gen)];
		}
		return true;
	}
	void randomMove(const BOARD &B, MOV &BestMove, const MOV &prohitbitMove){
		cerr << "[!] In Random Move: \n";
		assert(B.who != -1);
		vector<MOV> movList; // Candidates
		for(POS p = 0; p < 32; p++){
			FIN pf = B.fin[p];
			// 自己的棋才繼續
			if(GetColor(pf) != B.who) continue;
			// 搜尋四個方向
			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // 如果是不合法的步就跳過
				FIN qf = B.fin[q];
				// 是可以走吃的步 且 不可以是被禁止的步
				if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
					movList.push_back(MOV(p, q)); // 存下從 p 往 q 走的 move
				}
			}
		}
		
		if(movList.size() == 0){
			// Let it go 只好讓他和局
			BestMove = prohitbitMove;
			assert(prohitbitMove.st != -1 && prohitbitMove.ed != -1);
		}else{
			// random sample one move
			uniform_int_distribution<int> U(0, movList.size()-1);
			BestMove = movList[U(gen)];
		}
		cerr << "[*] BestMove: " << BestMove.st << " " << BestMove.ed << "\n";
		cerr << "[!] Random move end!" << "\n";
	}

	// 仔細思考
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		
		// 檢查 transposition table
		Record record; // 存 retrieve 出來的結果
		// 如果 B.who 還沒決定就不做
		if(B.who != -1){
			record = this->transTable[B.who].getVal(B.hashKey);
			if(record.val != nullptr){
				// cerr << "record value: " << *record.val  << " , flag: " 
				//   << *record.flag << " , remain depth: " << *record.depth << "\n";
				this->logger <<  "record value: " << *record.val  << " , flag: " \
				 << *record.flag << " , remain depth: " << *record.depth << "\n";
			}
		}
		if(record.val != nullptr){
			// 第一層不可直接回傳, 不然會找不到 BestMove
			if(depth != 0){
				// exact
				if(*record.flag == 0){
					// 檢查深度: 所剩深度 比 裡面的所剩深度還小
					if((this->cutOffDepth - depth) <= *record.depth){
						// 可以直接回傳
						return *record.val;
					}
				}
				// lower bound(把 alpha 拉高一點)
				else if(*record.flag == 1){
					if((this->cutOffDepth - depth) <= *record.depth){
						alpha = MAX(alpha, *record.val);
					}
				}
				// upper bound(把 beta 拉低一點)
				else if(*record.flag == 2){
					if((this->cutOffDepth - depth) <= *record.depth){
						beta = MIN(beta, *record.val);
					}
				}
				// check cutoff
				if(alpha >= beta){
					return beta;
				}
			}
		}
		
		MOVLST lst;
		// 只在 root 的時候 或 小於 3 子沒翻出來的時候 才模擬翻出子來的情況, 其他情況�m只對翻開來的子做搜尋
		// if(depth == 0 || B.sumCnt < 3){
		// 	B.MoveGenWithFlip(lst);
		// }else{
			B.MoveGen(lst);
		// 

		// * Time out
		// * If win (winner is not other)
		// * depth cut off
		// * no move can take
		timeval stop;
		gettimeofday(&stop, NULL);
		const int milliElapsed = (stop.tv_sec - start.tv_sec) * 1000 + (stop.tv_usec-start.tv_usec) / 1000;
		// 如果贏了就給很高分, 遠超過所有好子的分數
		const CLR winner = B.getWinner();
		if(winner != -1){
			// 贏棋時得到 最大 分數
			if(winner == B.who) return SearchEngine::vMax;
			// 輸棋時得到 最小 分數
			return SearchEngine::vMin;
		}
		if(milliElapsed > this->timeOut || depth == this->cutOffDepth 
			|| lst.num == 0){
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
				// TODO: 
				assert(false);
				// Chance Search()
				t = this->chanceSearch(B, lst.mov[i], MAX(alpha, m), beta, BestMove, depth);
			}
			// * Deterministic node
			else{
				nextB.DoMove(lst.mov[i], FIN_E); // 後面 FIN_E 用不到
				// Scout
				t = -this->negaScout(nextB, -n, -MAX(alpha, m), BestMove, depth+1);
				if(depth == 0){
					cerr << "lst.mov[i].st: " << lst.mov[i].st << " , lst.mov[i].ed" << lst.mov[i].ed << ", value: " << t <<"\n"; 
				}
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
					// 已經有存在 element 時
					if(record.val != nullptr){
						// 所剩深度比較深時, 更新值
						if((this->cutOffDepth - depth) >  *record.depth){
							*record.val = m;
							*record.flag = 1; // lower bound
							*record.depth = this->cutOffDepth - depth; // 紀錄所剩深度
						}
					} // lower bound
					else this->transTable[B.who].insert(B.hashKey, m, 1, this->cutOffDepth - depth);
					
					return m;
				}
			}
			n = MAX(alpha, m) + 1; // null window
		}
		// m in [alpha, beta]
		if(m > alpha){
			// element 存在時
			if(record.val != nullptr){
				// 檢查深度有沒有變深
				// remain 深度大於 hash entry 的
				if((this->cutOffDepth - depth) >  *record.depth){
					*record.val = m;
					*record.flag = 0;
					*record.depth = this->cutOffDepth - depth;
				}
			}else this->transTable[B.who].insert(B.hashKey, m, 0, this->cutOffDepth - depth);
		}else{
			if(record.val != nullptr){
				if((this->cutOffDepth - depth) >  *record.depth){
					*record.val = m;
					*record.flag = 2; // upper bound
					*record.depth = this->cutOffDepth - depth;
				}
			} // upper bound
			else this->transTable[B.who].insert(B.hashKey, m, 2, this->cutOffDepth - depth);
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

	// h(B): Nega version(return h(p), from the view of current Board)
	SCORE Eval(const BOARD &B) {
		assert(B.who != -1);
		SCORE cnt[2]={0,0}; // 0: 紅方, 1: 黑方
		// 計算翻開棋的子力分數
		for(POS p=0;p<32;p++){
			const CLR c = GetColor(B.fin[p]);
			// 如果不是未翻開的
			if(c != -1){
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
	// Sum of fin score
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
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200}; // http://www.csie.ntnu.edu.tw/~linss/Students_Thesis/2011_06_29_Lao_Yung_Hsiang.pdf
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore()+10000;
SCORE SearchEngine::vMin = -SearchEngine::vMax;
#endif