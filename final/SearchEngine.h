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
	static SCORE finScore[7]; // 0: ´”±N, ..., 6: ßL®Ú
	int cutOffDepth = 7; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start;
	TranspositionTable transTable[2]; // 0: red, 1: black
	TranspositionTable visitedTable; // ¿À¨d¶≥®S¶≥§T¥`¿Ù™∫ table
	fstream logger;

	SearchEngine(){
		this->logger.open("log.txt",  ios::out | ios::trunc);
		this->logger << "Start Search" << "\n";
	}
	~SearchEngine(){
		this->logger.close();
	}

	// ∞µ•X®M©w
	MOV Play(const BOARD &B, int remain_milliseconds){
		MOV BestMove(-1, -1);
		// Retrieve from visitedTable 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// ¶p™G§ß´e∞—≥XπL
			*record.flag += 1; // add visit time
		}else{
			this->visitedTable.insert(B.hashKey, -1, 1, -1);
		}
		// ∂}ßΩ Rule
		if(this->specialBoardCondition(B, BestMove)){
			return BestMove;
		}
		assert(B.who != -1); // B.who == -1 ∑|≥Q special condition æ◊±º
		// Pre-evaluation
		SCORE preScore = this->Eval(B);
		this->logger << "Prescore is: " << preScore << "\n" << flush;
		// NegaMax 
		// * ∂V¶h∑t§l™∫Æ…≠‘™·Æ…∂°§÷§@¬I
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
		
		// ¶p™G¶]¨∞®S¶≥ MOVLIST, ¶”®S¶≥®B™∫∏‹, ∂√¬Ω
		if(BestMove.st == -1 || BestMove.ed == -1){
			// ¶p™G®S¶≥≤æ∞ ®B™∫∏‹§@©w¨O¶≥§l•i•H¬Ω (∞£´D•d¶∫ ex. ®‚∞¶Ø••d¶∫ßL, ¶˝¨O≥o≠”∑|•˝≥Q Judge ßP©wµ≤ßÙ)
			assert(randomFlip(B, BestMove));
		}
		
		if(BestMove.st == BestMove.ed){
			// ¬Ω¥—§l§£∑|≥y¶® 3 ¥`¿Ù(¶]¨∞¬Ω•X®”§ß´·§@©w®SøÏ™k©π¶^®´)
			// ©“•H§£ª›≠n¨ˆø˝¥ø∏g∞—≥X, ¶]¨∞§@©w§£∑|≠´Ω∆ΩL≠±
		}
		// ¶p™G¨O®´§l¥N≠n¿À¨d∑|§£∑|≠´Ω∆
		else{
			cerr << "[*] Checking Repetition......\n";
			// FIXME: ∑|•d¶Ì°H°H
			// ¶p™G§£¨O¬Ω¥—°A≠n¶“º{¨›¨›¶≥®S¶≥•iØ‡≥y¶® 3 ¥`¿Ù
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15)); // FIN(15) §£∑|•Œ®Ï
			record = this->visitedTable.getVal(nextB.hashKey);
			// ¶p™G flag §w∏g¨O 2 §F, ¥N§£≠n¶A®´≥o§@®B, ßÔ¶®∂√¬Ω©Œ¨O∂√®´
			if(record.flag != nullptr && (*record.flag) >= 1){
				cerr << "[*] visit count: " << *record.flag << "\n";
				// ¶p™G¡Ÿ¶≥§l•i•H¬Ω
				if(B.sumCnt > 0) {
					// ¬Ω§lµ¥πÔ§£∑|≥y¶®§T¥`¿Ù
					assert(this->randomFlip(B, BestMove)); // §@©w∑|•i•H¬Ω
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove); // ∂√®´, ¶˝§£Ø‡®´®Ï prohibit move
					assert(BestMove.st != BestMove.ed);
					
				}
			}

			// (BestMove ¶≥•iØ‡§w∏g¥´§F)¶p™G≥Ã´·øÔ•X®”¨O®´¶Y®B, ¥N∞O¶Ì§U§@®B∞—≥X¶∏º∆, ¶p™G¨O¬Ω§l®B™∫∏‹¥N§£•Œ∞O§F°A¶]¨∞¬Ω§l§£∑|≥y¶®¥`¿Ù
			if(BestMove.st != BestMove.ed){
				nextB = B;
				nextB.DoMove(BestMove, FIN(15));
				record = this->visitedTable.getVal(nextB.hashKey);
				if(record.flag != nullptr){
					*record.flag += 1;
					assert(*record.flag < 32767); // ¿≥∏”§£Ø‡ overflow
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
		// * who ¡Ÿ®S®M©wÆ… || B.sumCnt == 32
		if(B.who == -1 || B.sumCnt == 32) {
			assert(randomFlip(B, BestMove)); // ¿H´K¬Ω
			return true;
		}
		// * ¶p™GπÔ§ËØ•©Œ§˝§w∏g•X≤{Æ…, ¥N™Ω±µ¬Ω•LÆ«√‰(≠n∞˜¶h∑t§lÆ…§~∞µ≥o•Û®∆)
		assert(B.who != -1);
		const CLR oppColor = B.who^1;
		if((B.sumCnt > 28) && (B.cnt[FIN_C + oppColor*7] < 2 || B.cnt[FIN_K + oppColor*7] == 0)){
			assert(randomFlip(B, BestMove));
			return true;
		}
		
		// * ¬i≠±§W•ÿ´eêm¨OßO§H™∫§l™∫Æ…≠‘
		// * ¬i≠±§W•ÿ´eêm¡Ÿ¨O¶€§v™∫§l™∫Æ…≠‘
		return false;
	}
	
	bool randomFlip(const BOARD &B, MOV &BestMove){
		// * §£≠n¬Ω®ÏßO§H™∫§lÆ«√‰
		// * ∞£§F¶p™Gºƒ§H¬Ω®ÏØ•©Œ´”, ™Ω±µ¬Ω•LÆ«√‰∑F±º•L
		if(B.sumCnt==0) return false;
		vector<MOV> movList;
		vector<MOV> dangerList;
		CLR oppColor = (B.who == -1)? (-1):(B.who^1) ; // 
		// ±ΩπL©“¶≥ position
		for(POS p = 0; p < 32; p++){
			bool isDangerous = false;
			// ¶p™G≥o≠”§l§£¨O∑t§l™∫∏‹¥N∏ıπL
			if(B.fin[p] != FIN_X) continue;
			// ¿À¨d≥o≠”§l™∫•|≠±§K§Ë¶≥®S¶≥ºƒ§H™∫§l
			for(int dir=0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				// ¶p™G§£¨O¶X≤z™∫®B¥N∏ıπL
				if(q == -1) continue;
				// ¶p™G¨Oºƒ§Ë™∫§l, ∫…∂q§£≠n¬ΩÆ«√‰(∞£§FØ•©Œ´”), ¶p™G¨O unknown √C¶‚•N™Ìêm•i•H¬Ω
				if((oppColor != -1) && (GetColor(B.fin[q]) == oppColor)){
					isDangerous = true;
					// ¶p™G¨OØ• ©Œ ´”, êm™Ω±µ¬Ω•Lπjæ¿, Ω‰•i•Hß‚•¶®≥≥t¶Y±º
					if((GetLevel(B.fin[q]) == LVL_C) || (GetLevel(B.fin[q]) == LVL_K)){
						BestMove = MOV(p, p);
						return true;
					}
				}
			}
			// ¶p™G§£¶M¿I§~ push back
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
			// ¶p™G§£¨O¶€§v™∫¥—, ¥N∏ıπL
			if(GetColor(pf) != B.who) continue;
			// ∑j¥M•|≠”§Ë¶V
			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // ¶p™G¨O§£¶X™k™∫®B¥N∏ıπL
				FIN qf = B.fin[q];
				// ¨O•i•H®´¶Y™∫®B •B §£•i•H¨O≥Q∏T§Ó™∫®B
				// ¶p™G§£¨O¨∂°A•i•H™Ω±µßP¬_Ø‡§£Ø‡¶Yπjæ¿™∫§l
				if(GetLevel(pf) != LVL_C){
					if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
						movList.push_back(MOV(p, q)); // ¶s§U±q p ©π q ®´™∫ move
					}
				}else{
					// ¿À¨d¨∂™∫®´¶Ï
					int countFinAlongDir = 0;
					if(qf == FIN_E) movList.push_back(MOV(p, q)); // ¶p™G¨O™≈§l™∫∏‹, ¥N•i•H®´
					// ©π§@≠”™ΩΩu±ΩπL•h, ¶p™G qLocal ¡Ÿ§£¨O -1 •B §§∂°™∫§l§¥µM§p©Ûµ•©Û 1 ¡˚§l°A¥N•i•Hƒ~ƒÚ
					for(POS qLocal = ADJ[q][dir]; (qLocal != -1) && (countFinAlongDir <= 1) ; 
						qLocal = ADJ[qLocal][dir]){
						const FIN qfLocal = B.fin[qLocal];
						// ¶p™G§§∂°πj§@≠”§l ¶”•B ∑Ì´e qfLocal ¨OπÔ§Ë√C¶‚™∫§l, ´h•i•H¶Y§l
						if(countFinAlongDir == 1 && GetColor(qfLocal) == (B.who^1) ){
							movList.push_back(MOV(p, qLocal));
							this->logger << "Cannon Move: (" << p << "," << qLocal << ")\n";
						}
						// ¶p™G≤{¶b≥o≠”§l¨O √C¶‚§l©Œ∑t§l, ¥N≠p§J countFinAlongDir
						if(qfLocal <= FIN_X){
							countFinAlongDir++;
						}
					}
				}
			}
		}
		
		if(movList.size() == 0){
			// Let it go •u¶n≈˝•L©MßΩ
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

	// •J≤”´‰¶“
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		
		// ¿À¨d transposition table
		Record record; // ¶s retrieve •X®”™∫µ≤™G
		// ¶p™G B.who ¡Ÿ®S®M©w¥N§£∞µ
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
			// ≤ƒ§@ºh§£•i™Ω±µ¶^∂«, §£µM∑|ß‰§£®Ï BestMove
			if(depth != 0){
				// exact
				if(*record.flag == 0){
					// ¿À¨d≤`´◊: ©“≥—≤`´◊ §Ò ∏Ã≠±™∫©“≥—≤`´◊¡Ÿ§p
					if((this->cutOffDepth - depth) <= *record.depth){
						// •i•H™Ω±µ¶^∂«
						return *record.val;
					}
				}
				// lower bound(ß‚ alpha ©‘∞™§@¬I)
				else if(*record.flag == 1){
					if((this->cutOffDepth - depth) <= *record.depth){
						alpha = MAX(alpha, *record.val);
					}
				}
				// upper bound(ß‚ beta ©‘ßC§@¬I)
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
		// •u¶b root ™∫Æ…≠‘ ©Œ §p©Û 3 §l®S¬Ω•X®”™∫Æ…≠‘ §~º“¿¿¬Ω•X§l®”™∫±°™p, ®‰•L±°™pêm•uπÔ¬Ω∂}®”™∫§l∞µ∑j¥M
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
		// ¶p™Gƒπ§F¥Nµπ´‹∞™§¿, ª∑∂WπL©“¶≥¶n§l™∫§¿º∆
		const CLR winner = B.getWinner();
		if(winner != -1){
			// ƒπ¥—Æ…±o®Ï ≥Ã§j §¿º∆
			if(winner == B.who) return SearchEngine::vMax;
			// øÈ¥—Æ…±o®Ï ≥Ã§p §¿º∆
			return SearchEngine::vMin;
		}
		if(milliElapsed > this->timeOut || depth == this->cutOffDepth 
			|| lst.num == 0){
			this->logger << "[*] Scout Depth: " << depth << "\n";
			this->logger << "[*] Board: " << B.who << "\n";
			this->logger << "[*] Elapse seconds: " << milliElapsed / 1000 << "\n";
			return Eval(B);
		}
		// FIXME:¥˙∏’§@§U
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
				nextB.DoMove(lst.mov[i], FIN_E); // ´·≠± FIN_E •Œ§£®Ï
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
					// §w∏g¶≥¶s¶b element Æ…
					if(record.val != nullptr){
						// ©“≥—≤`´◊§Ò∏˚≤`Æ…, ßÛ∑s≠»
						if((this->cutOffDepth - depth) >  *record.depth){
							*record.val = m;
							*record.flag = 1; // lower bound
							*record.depth = this->cutOffDepth - depth; // ¨ˆø˝©“≥—≤`´◊
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
			// element ¶s¶bÆ…
			if(record.val != nullptr){
				// ¿À¨d≤`´◊¶≥®S¶≥≈‹≤`
				// remain ≤`´◊§j©Û hash entry ™∫
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
		// ¡Ÿ¶≥®S¶≥¬Ω•X®”™∫¥—
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
			// ¶p™G®∫≠”¥—§lêm§w∏g¬Ω∂}§F
			if(B.cnt[i] == 0){
				continue;
			}
			// ¶p™G≥o≠”ßL∫ÿ¡Ÿ¶≥¶s¶b™∫∏‹
			else{
				nextB = B;
				// ∞≤≥]≥o≠”ßL∫ÿ≥Q¬Ω∂}®”
				nextB.DoMove(mv, FIN(i));
				// 
				if(!isfirst){
					// (c / w1) * (alpha - vMax)
					tmpLocalRatio = ((double)B.sumCnt / (double)B.cnt[i]);
					// ∑|§£∑|¶≥ overflow ™∫∞›√D°H
					A = tmpLocalRatio * (alpha - SearchEngine::vMax) + SearchEngine::vMax;
					B_ = tmpLocalRatio * (beta - SearchEngine::vMin) + SearchEngine::vMin;
					isfirst = false;
				}
				// TODO: ≠n•[≠t∏π∂‹°H ß⁄ƒ±±o¿≥∏”≠n•[¶b≥o∏Ã, ¶]¨∞ [alpha, beta] ≤{¶b¨O∞wπÔ≥o≠” node ™∫
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
		SCORE cnt[2]={0,0}; // 0: ¨ı§Ë, 1: ∂¬§Ë
		// ≠p∫‚¬Ω∂}¥—™∫§l§O§¿º∆
		for(POS p=0;p<32;p++){
			const CLR c = GetColor(B.fin[p]);
			// ¶p™G§£¨O•º¬Ω∂}™∫
			if(c != -1){
				// ≠p∫‚§l§O§¿º∆
				cnt[c] += SearchEngine::finScore[GetLevel(B.fin[p])];
			}
		}
		// ≠p∫‚•º¬Ω∂}™∫§l§O§¿º∆
		for(int i=0;i<14;i++){
			cnt[i / 7] += (double)B.cnt[i] * SearchEngine::finScore[i % 7];
		}
		// MIN node: ∂V∞™•N™ÌπÔ max node ∂V§£ßQ
		// Max node: ∂V∞™•N™ÌπÔ min node ∂V§£ßQ
		// TODO: ∂Z¬˜§]≠n∫‚§@¬I§¿º∆°I°I°I
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
// §l§O§¿º∆(0: ´”, ..., 6:®Ú)
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200}; // http://www.csie.ntnu.edu.tw/~linss/Students_Thesis/2011_06_29_Lao_Yung_Hsiang.pdf
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore()+10000;
SCORE SearchEngine::vMin = -SearchEngine::vMax;
#endif