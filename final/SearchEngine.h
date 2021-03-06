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
	int cutOffDepth = 8; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start;
	TranspositionTable transTable[2]; // 0: red, 1: black
	TranspositionTable visitedTable; // 檢查有沒有三循環的 table
	fstream logger;

	SearchEngine(){
		this->logger.open("log.txt",  ios::out | ios::trunc);
		this->logger << "Start Search" << "\n";
		#ifdef ATTACKMOVE
		cerr << "Using Attack Move\n";
		#endif
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
		// 如果別人第一子翻出 王
		if((B.sumCnt == 31) && (B.cnt[FIN_C + oppColor*7] < 2 || B.cnt[FIN_K + oppColor*7] == 0)){
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
			// 如果不是自己的棋, 就跳過
			if(GetColor(pf) != B.who) continue;
			// 搜尋四個方向
			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // 如果是不合法的步就跳過
				FIN qf = B.fin[q];
				// 是可以走吃的步 且 不可以是被禁止的步
				// 如果不是炮，可以直接判斷能不能吃隔壁的子
				if(GetLevel(pf) != LVL_C){
					if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
						movList.push_back(MOV(p, q)); // 存下從 p 往 q 走的 move
					}
				}else{
					// 檢查炮的走位
					int countFinAlongDir = 0;
					if(qf == FIN_E) movList.push_back(MOV(p, q)); // 如果是空子的話, 就可以走
					// 往一個直線掃過去, 如果 qLocal 還不是 -1 且 中間的子仍然小於等於 1 顆子，就可以繼續
					for(POS qLocal = ADJ[q][dir]; (qLocal != -1) && (countFinAlongDir <= 1) ; 
						qLocal = ADJ[qLocal][dir]){
						const FIN qfLocal = B.fin[qLocal];
						// 如果中間隔一個子 而且 當前 qfLocal 是對方顏色的子, 則可以吃子
						if(countFinAlongDir == 1 && GetColor(qfLocal) == (B.who^1) ){
							movList.push_back(MOV(p, qLocal));
							this->logger << "Cannon Move: (" << p << "," << qLocal << ")\n";
						}
						// 如果現在這個子是 顏色子或暗子, 就計入 countFinAlongDir
						if(qfLocal <= FIN_X){
							countFinAlongDir++;
						}
					}
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
		// alpha 有可能 大於 beta, 
		if(alpha > beta){
			cerr << "Why alpha < beta???\n" << "alpha: " << alpha << " " << beta << "\n";
			assert(false);
		}
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
					// 理論上：假設 A node 底下有一步會走到 A node, 那第一次搜的時候 下面的 A node 記在 hash 的值會被蓋掉
					// 所以最後留下來的 hash 值會是 root A node
					// 因此，第二次搜的時候，如果底下這一行使用 <= 的話，會造成底下的 A node 使用的是 第一次搜尋時 root A node 的值
					// 所以第二次搜尋才會造成不同結果！！！(assert(*record.val == m) 會錯的原因)
					// 實測 70 幾場發現如果只用這個應該就不會噴了！。
					if((this->cutOffDepth - depth) == *record.depth){
						// 可以直接回傳
						return *record.val; // 這一步可能會造成深度不同 而有不同的值？
					}
				}
				// lower bound(把 alpha 拉高一點)
				// 理論上：第一次 A node 底下往下搜的時候，可能底下的重複盤面 A node 有被 cache 住(但是是比較淺層的)，因此 root A node 搜的時候, 會使用到淺層 cache 住的值, 而這個 cache 的值可能會造成一開始 alpha 就設得比較高（同時 這個 cache 值可能是從第 4 個 branch 找到的，但一開始就設定為 alpha 可能會造成前面的 branch 噴出不同噴出不同的值
				// 但第二次 A node 搜的時候，再一次碰到底下的重複盤面 A node 時，因為 A node 已經被深一層的值換掉了，因此用不到 cache 的值了，所以使用的 alpha 是他的 parent 傳來的
				// 結論：循環盤面時用 TTB 要很小心！！！！
				// else if(*record.flag == 1){
				// 	// 這邊也要改成 ==，以避免 negaScout 中有 loop
				// 	if((this->cutOffDepth - depth) == *record.depth){
				// 		alpha = MAX(alpha, *record.val); 
				// 	}
				// }
				// // upper bound(把 beta 拉低一點)
				// else if(*record.flag == 2){
				// 	// 這邊也要改成 ==，以避免 negaScout 中有 loop
				// 	if((this->cutOffDepth - depth) == *record.depth){
				// 		beta = MIN(beta, *record.val);
				// 	}
				// }
				// check cutoff
				// if(alpha >= beta){
				// 	return *record.val; // fail-soft
				// 	return alpha; // assert(*record.val == m) bugs occur at here!!!! Because we need to return fail-soft value
				// }
				// // 好像如果 alpha, beta 如果 bound 沒有衝到，應該要用回原來的 alpha beta, 不然好像(不是很確定)會造成 assert(m == *record.val) 噴掉?
				// else{
				// 	// 還原 alpha, beta(如果之前有改到)
				// 	// 經過實測(100場)發現這樣加就不會 assert(*record.val == m) 噴掉
				// 	// 我覺得是可能是因為如果你把 alpha, beta bound 變窄時，丟下去 Scout 或 negaMax 搜的時候拿到的值可能會不一樣(因為提到 break 的時間點會不一樣)
				// 	// 所以造成 assert(*record.val == m) 會噴掉，因為你可能拿到的 Scout, negaMax 值會不同
				// 	if(*record.flag == 1 && (this->cutOffDepth - depth) <= *record.depth){
				// 		alpha = alphaOld;
				// 	}else if(*record.flag == 2 && (this->cutOffDepth - depth) <= *record.depth){
				// 		beta = betaOld;
				// 	}
				// }
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
		// QQ timeout will cause transposition table wrong............
		if(depth == this->cutOffDepth 
			|| lst.num == 0){
			this->logger << "[*] Scout Depth: " << depth << "\n";
			this->logger << "[*] Board: " << B.who << "\n";
			this->logger << "[*] Elapse seconds: " << milliElapsed / 1000 << "\n";
			return Eval(B);
		}
		// FIXME:測試一下
		SCORE m = SearchEngine::vMin-1, n = beta;
		SCORE t;
		BOARD nextB;
		for(int i = 0; i < lst.num; i++){
			nextB = B;
			// * Chance node
			// TODO:
			if(lst.mov[i].st == lst.mov[i].ed){
				// TODO: 
				// Chance Search()
				t = this->chanceSearch(B, lst.mov[i], MAX(alpha, m), beta, BestMove, depth);
			}
			// * Deterministic node
			else{
				nextB.DoMove(lst.mov[i], FIN_E); // 後面 FIN_E 用不到
				// Scout
				t = -this->negaScout(nextB, -n, -MAX(alpha, m), BestMove, depth+1); 
				if(depth == 0){
					
					cerr << "\n";
					cerr << "alpha: " << alpha << " ,beta: " << beta << "\n";
					cerr << "now m: " << m <<  ", attack flag: " << ((lst.mov[i].isAttack)? ('t'):('f')) << "\n";
					cerr << "scout: lst.mov[i].st: " << lst.mov[i].st << " , lst.mov[i].ed" << lst.mov[i].ed << ", value: " << t <<"\n"; 
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
					// 如果這一步是攻擊步(ed 是對方顏色的子), 直接加 100 分(100 分
					// 不超過一個兵卒的分數，因此順序不會變，但是原本同樣好的步會因此分出高下)
					#ifdef ATTACKMOVE
					// FIXME: BUG 因為如果循環盤面的話, 因為 transposition table caching 的關係, 會讓這一步 attack move 加完的值被存在 table 裡面造成誤判
					if(GetColor(B.fin[lst.mov[i].ed]) == (B.who^1)){
						m += 100;
						cerr << "[*] Attack Move!\n";
					}
					#endif
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
			n = MAX(alpha, m) + 0.1; // 還是因為 null window 原本設成 +1 的關係
		}
		// m in [alpha, beta]
		if(m > alpha){
			// element 存在時
			if(record.val != nullptr){
				// 如果是 flag 是 exact, 那應該要一樣
				if(*record.flag == 0 && (this->cutOffDepth - depth) == *record.depth){
					cerr << "What the hell?\n";
					cerr << "alpha: " << alpha << " ,beta: " << beta << "\n";
					cerr << "record.val: " << *record.val << " , now m: " << m << "\n";
					// TODO: 後來實測發現, 這邊還是會噴, 我覺得可能是因為 lower bound 
					// 跟 upper bound flag 有機率把 alpha beta bound 推高(這段 function 的最前面), 以至於回傳的值有可能跟以前不同
					// P.S. (這只是我的推測, 如果有強�耵器D為什麼再麻煩告訴我了)
					// 後來再實測發現，其實要撞到這個 bug 頗難的, 嚴重懷疑原因可能是因為 s[][], color[] 可能有撞到 hash 值
					// 後來再發現，如果 TTB 回傳是 fail-soft, 好像還是會有這個問題，開始在想會不會是因為 alpha, beta 被 TTB cache調過之後造成不同的 behavior
					// 後來又再思考，會不會是因為循環盤面所造成的 ---> 結論是：應該是了
					assert(m == *record.val); // 如果深度一樣, 得到的值必須一樣
					// 使用最新的值!!!!!
					*record.val = m;
				}
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
				// 用最深的深度蓋過去
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
		double A=0, B_=0;
		double m=SearchEngine::vMin, M = SearchEngine::vMax;
		double t=0;
		double tmpLocalRatio;
		double weightVSum = 0;
		bool isfirst = true;
		BOARD nextB;
		// 
		for(int i = 0; i < 14; i++){
			// 如果那個棋子�m已經翻開了
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
					// 會不會有 overflow 的問題？
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
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore()+10000; // 勝的時候應該要比所有分數�m高
SCORE SearchEngine::vMin = -SearchEngine::vMax;
#endif