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
	static SCORE finScore[7]; // 0: «Ó±N, ..., 6: §L¨ò
	int cutOffDepth = -1; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start;
	TranspositionTable transTable[2]; // 0: red, 1: black
	TranspositionTable visitedTable; // ÀË¬d¦³¨S¦³¤T´`Àôªº table
	fstream logger;

	SearchEngine(){
		this->logger.open("log.txt",  ios::out | ios::trunc);
		this->logger << "Start Search" << "\n";
	}
	~SearchEngine(){
		this->logger.close();
	}

	// °µ¥X¨M©w
	MOV Play(const BOARD &B, int remain_milliseconds){
		MOV BestMove(-1, -1);
		// 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// ¦pªG¤§«e°Ñ³X¹L
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
		// * ¶V¦h·t¤lªº®É­Ôªá®É¶¡¤Ö¤@ÂI
		// this->timeOut = remain_milliseconds * ((33 - B.sumCnt) / 32) - 1;
		this->timeOut = 5000;
		
		gettimeofday(&this->start, NULL);
		cerr << "[*] Start negaScout" << "\n";
		// ID NegaScout
		SCORE negaScore;
		for(int i = 0; i < 12; i++){
			this->cutOffDepth = i;
			timeval stop;
			gettimeofday(&stop, NULL);
			const int milliElapsed = (stop.tv_sec - start.tv_sec) * 1000 + (stop.tv_usec-start.tv_usec) / 1000;
			negaScore = this->negaScout(B, SearchEngine::vMin, SearchEngine::vMax, BestMove, 0);
			if(milliElapsed > this->timeOut){
				break;
			}
		}
		cerr << "Negascore: " << negaScore << "\n";
		cerr << "[*] End negaScout" << "\n";
		
		// ¦pªG negaScout ·j¤£¥X¨B¨Ó
		if(BestMove.st == -1 || BestMove.ed == -1){
			assert(randomFlip(B, BestMove));
		}
		
		if(BestMove.st == BestMove.ed){
			// Â½´Ñ¤l¤£·|³y¦¨ 3 ´`Àô(¦]¬°Â½¥X¨Ó¤§«á¤@©w¨S¿ìªk©¹¦^¨«)
			// ©Ò¥H¤£»İ­n¬ö¿ı´¿¸g°Ñ³X, ¦]¬°¤@©w¤£·|­«½Æ½L­±
		}
		else{
			cerr << "[*] Checking Repetition......\n";
			// FIXME: ·|¥d¦í¡H¡H
			// ¦pªG¤£¬OÂ½´Ñ¡A­n¦Ò¼{¬İ¬İ¦³¨S¦³¥i¯à³y¦¨ 3 ´`Àô
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15));
			cerr << "[*] nextB.hashKey: " << bitset<64>(nextB.hashKey) << "\n";
			record = this->visitedTable.getVal(nextB.hashKey);
			// ¦pªG flag ¤w¸g¬O 2 ¤F, ¨º´N¶Ã¨«
			if(record.flag != nullptr) this->logger << "[*] This Board has been visited: " << *record.flag << "\n";
			if(record.flag != nullptr && (*record.flag) >= 2){
				if(B.sumCnt > 0) {
					// Â½¤lµ´¹ï¤£·|³y¦¨¤T´`Àô
					this->randomFlip(B, BestMove);
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove);
					assert(BestMove.st != BestMove.ed);
					
				}	
			}			
			// ¦pªG³Ì«á¿ï¥X¨Ó¬O¨«¦Y¨B, ´N°O¤@¤U°Ñ³X¹L
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
		// * who ÁÙ¨S¨M©w®É
		if(B.who == -1) {
			assert(randomFlip(B, BestMove));
			return true;
		}
		// * ¥ş³¡mÁÙ¨SÂ½®É
		if(B.sumCnt == 32){
			// ³o¨B¤@©w·|§ïÅÜ BestMove
			assert(randomFlip(B, BestMove));
			return true;
		}
		// * ¦pªG¹ï¤è¯¥¤w¸g¥X²{, ¦Ó¥B
		
		// * ¦pªG«Ó¤w¸g¥X²{, ¦Ó¥B§Úªº§L¨òmÁÙ¦³¤­­Óªº®É­Ô
		
		// * Âi­±¤W¥Ø«em¬O§O¤Hªº¤lªº®É­Ô
		// * Âi­±¤W¥Ø«emÁÙ¬O¦Û¤vªº¤lªº®É­Ô
		return false;
	}
	
	bool randomFlip(const BOARD &B, MOV &BestMove){
		
		// * ¤£­nÂ½¨ì§O¤Hªº¤l®ÇÃä
		// * °£¤F¦pªG¼Ä¤HÂ½¨ì¯¥©Î«Ó, ª½±µÂ½¥L®ÇÃä·F±¼¥L
		if(B.sumCnt==0) return false;
		vector<MOV> movList;
		vector<MOV> dangerList;
		CLR oppColor = (B.who^1); // ¦pªG B.who == -1, B.who^1 ·|ÅÜ¦¨ -2, ¤£¼vñ
		
		for(POS p = 0; p < 32; p++){
			cerr << "p: "<< p << "\n";
			// ¦pªG³o­Ó¤l¬O·t¤l
			bool isDangerous = false;
			if(B.fin[p] == FIN_X){
				//ÀË¬d¥|­±¤K¤è¦³¨S¦³¼Ä¤Hªº¤l
				for(int dir=0; dir < 4; dir++){
					POS q = ADJ[p][dir];
					// ¦pªG¬O¼Ä¤èªº¤l, ºÉ¶q¤£­nÂ½®ÇÃä(°£¤F¯¥©Î«Ó)
					if(GetColor(B.fin[q]) == oppColor){
						isDangerous = true;
						// ¦pªG¬O¯¥ ©Î «Ó, mª½±µÂ½¥L¹j¾À
						if(GetLevel(B.fin[q]) == LVL_C && GetLevel(B.fin[q]) == LVL_K){
							BestMove = MOV(p, p);
							return true;
						}
					}
				}
				// ¦pªG¤£¦MÀI¤~ push back
				if(!isDangerous) movList.push_back(MOV(p, p));
				else dangerList.push_back(MOV(p, p));
			}
		}
		
		if(movList.size() == 0){
			// ¦pªG³s danger list m¨S¦³ªº¸Ü
			if(dangerList.size() == 0) return false;
			uniform_int_distribution<int> U(0, dangerList.size()-1);
			BestMove = dangerList[U(gen)];
		}else{
			uniform_int_distribution<int> U(0, movList.size()-1);
			BestMove = movList[U(gen)];
		}
		
		return true;
	}
	void randomMove(const BOARD &B, MOV &BestMove, const MOV &prohitbitMove){
		cerr << "[!] In randomMove: \n";
		vector<MOV> movList; // population
		for(POS p = 0; p < 32; p++){
			FIN pf = B.fin[p];
			// ¦Û¤vªº´Ñ¤~Ä~Äò
			if(GetColor(pf) != B.who) continue;

			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // ¦pªG¬O¤£¦Xªkªº¨B´N¸õ¹L
				FIN qf = B.fin[q];
				// ¬O¥i¥H¨«¦Yªº¨B  ¥B ¤£¥i¥H¬O³Q¸T¤îªº¨B
				if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
					movList.push_back(MOV(p, q));
					// (ChkEats ·|ÀË¬d¬O§_¬OªÅ©Î¹ï¤è¤l)¦pªG¹ï¤è¬O¤ı, ·F±¼¦A»¡
					if(GetColor(qf) == (B.who^1) && GetLevel(qf) == LVL_K){
						BestMove = MOV(p, q);
						return;
					}
				}
			}
		}
		// FIXME: I think i may happen
		if(movList.size() == 0){
			// Let it go ¥u¦nÅı¥L©M§½
			BestMove = prohitbitMove;
		}else{
			// random sample one move
			uniform_int_distribution<int> U(0, movList.size()-1);
			BestMove = movList[U(gen)];
		}
	}

	// ¥J²Ó«ä¦Ò
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		// ÀË¬d transposition table
		Record record; // ¦s retrieve ¥X¨Óªºµ²ªG
		if(B.who != -1){
			record = this->transTable[B.who].getVal(B.hashKey);
			if(record.val != nullptr){
				cerr << "record value: " << *record.val  << " , flag: " \
				 << *record.flag << " , remain depth" << *record.depth << "\n";
			}
		}
		if(record.val != nullptr){
			// ²Ä¤@¼h¤£¥iª½±µ¦^¶Ç, ¤£µM·|§ä¤£¨ì BestMove
			if(depth != 0){
				// exact
				if(*record.flag == 0){
					// ÀË¬d²`«×: ©Ò³Ñ²`«× ¤ñ ¸Ì­±ªº©Ò³Ñ²`«×ÁÙ¤p
					if((this->cutOffDepth - depth) <= *record.depth ){
						// ¥i¥Hª½±µ¦^¶Ç
						return *record.val;
					}
				}
				// lower bound(§â alpha ©Ô°ª¤@ÂI)
				if(*record.flag == 1){
					if((this->cutOffDepth - depth) <= *record.depth ){
						alpha = MAX(alpha, *record.val);
					}
				}
				// upper bound(§â beta ©Ô§C¤@ÂI)
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
		// ¥u¦b root ªº®É­Ô ©Î ¤p©ó 3 ¤l¨SÂ½¥X¨Óªº®É­Ô ¤~¼ÒÀÀÂ½¥X¤l¨Óªº±¡ªp, ¨ä¥L±¡ªpm¥u¹ïÂ½¶}¨Óªº¤l°µ·j´M
		// if(depth == 0 || B.sumCnt < 3){
		// 	B.MoveGenWithFlip(lst);
		// }else{
			B.MoveGen(lst);
		// }
		// * Time out
		// * If win (winner is not other)
		// * depth cut off
		// * no move can take
		timeval stop;
		gettimeofday(&stop, NULL);
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
				nextB.DoMove(lst.mov[i], FIN_E); // «á­± FIN_E ¥Î¤£¨ì
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
					if(record.val != nullptr){
						// ©Ò³Ñ²`«×¤ñ¸û²`®É
						if((this->cutOffDepth - depth) >  *record.depth){
							*record.val = m;
							*record.flag = 1; // lower bound
							*record.depth = this->cutOffDepth - depth;
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
			// element ¦s¦b®É
			if(record.val != nullptr){
				// ÀË¬d²`«×¦³¨S¦³ÅÜ²`
				// remain ²`«×¤j©ó hash entry ªº
				if((this->cutOffDepth - depth) >  *record.depth){
					*record.val = m;
					*record.flag = 0;
					*record.depth = this->cutOffDepth - depth;
				}
			}else{
				// ª½±µ´¡¤J­È
				this->transTable[B.who].insert(B.hashKey, m, 0, this->cutOffDepth - depth);
			}
			
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
		// ÁÙ¦³¨S¦³Â½¥X¨Óªº´Ñ
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
			// ¦pªG¨º­Ó´Ñ¤l¨S¦³¤F
			if(B.cnt[i] == 0){
				continue;
			}
			// ¦pªG³o­Ó§LºØÁÙ¦³¦s¦bªº¸Ü
			else{
				nextB = B;
				// °²³]³o­Ó§LºØ³QÂ½¶}¨Ó
				nextB.DoMove(mv, FIN(i));
				// 
				if(!isfirst){
					// (c / w1) * (alpha - vMax)
					tmpLocalRatio = ((double)B.sumCnt / (double)B.cnt[i]);
					A = tmpLocalRatio * (alpha - SearchEngine::vMax) + SearchEngine::vMax;
					B_ = tmpLocalRatio * (beta - SearchEngine::vMin) + SearchEngine::vMin;
					isfirst = false;
				}
				// TODO: ­n¥[­t¸¹¶Ü¡H §ÚÄ±±oÀ³¸Ó­n¥[¦b³o¸Ì, ¦]¬° [alpha, beta] ²{¦b¬O°w¹ï³o­Ó node ªº
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
		SCORE cnt[2]={0,0}; // 0: ¬õ¤è, 1: ¶Â¤è
		// ­pºâÂ½¶}´Ñªº¤l¤O¤À¼Æ
		for(POS p=0;p<32;p++){
			const CLR c=GetColor(B.fin[p]);
			// ¦pªG¤£¬O¥¼Â½¶}ªº
			if(c!=-1){
				// ­pºâ¤l¤O¤À¼Æ
				cnt[c] += SearchEngine::finScore[GetLevel(B.fin[p])];
			}
		}
		// ­pºâ¥¼Â½¶}ªº¤l¤O¤À¼Æ
		for(int i=0;i<14;i++){
			cnt[i / 7] += (double)B.cnt[i] * SearchEngine::finScore[i % 7];
		}
		// MIN node: ¶V°ª¥Nªí¹ï max node ¶V¤£§Q
		// Max node: ¶V°ª¥Nªí¹ï min node ¶V¤£§Q
		// TODO: ¶ZÂ÷¤]­nºâ¤@ÂI¤À¼Æ¡I¡I¡I
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
// ¤l¤O¤À¼Æ(0: «Ó, ..., 6:¨ò)
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200};
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore();
SCORE SearchEngine::vMin = -SearchEngine::vMin;
#endif