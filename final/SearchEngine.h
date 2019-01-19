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
	int cutOffDepth = 8; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start;
	TranspositionTable transTable[2]; // 0: red, 1: black
	TranspositionTable visitedTable; // ÀË¬d¦³¨S¦³¤T´`Àôªº table
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

	// °µ¥X¨M©w
	MOV Play(const BOARD &B, int remain_milliseconds){
		MOV BestMove(-1, -1);
		// Retrieve from visitedTable 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// ¦pªG¤§«e°Ñ³X¹L
			*record.flag += 1; // add visit time
		}else{
			this->visitedTable.insert(B.hashKey, -1, 1, -1);
		}
		// ¶}§½ Rule
		if(this->specialBoardCondition(B, BestMove)){
			return BestMove;
		}
		assert(B.who != -1); // B.who == -1 ·|³Q special condition ¾×±¼
		// Pre-evaluation
		SCORE preScore = this->Eval(B);
		this->logger << "Prescore is: " << preScore << "\n" << flush;
		// NegaMax 
		// * ¶V¦h·t¤lªº®É­Ôªá®É¶¡¤Ö¤@ÂI
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
		
		// ¦pªG¦]¬°¨S¦³ MOVLIST, ¦Ó¨S¦³¨Bªº¸Ü, ¶ÃÂ½
		if(BestMove.st == -1 || BestMove.ed == -1){
			// ¦pªG¨S¦³²¾°Ê¨Bªº¸Ü¤@©w¬O¦³¤l¥i¥HÂ½ (°£«D¥d¦º ex. ¨â°¦¯¥¥d¦º§L, ¦ı¬O³o­Ó·|¥ı³Q Judge §P©wµ²§ô)
			assert(randomFlip(B, BestMove));
		}
		
		if(BestMove.st == BestMove.ed){
			// Â½´Ñ¤l¤£·|³y¦¨ 3 ´`Àô(¦]¬°Â½¥X¨Ó¤§«á¤@©w¨S¿ìªk©¹¦^¨«)
			// ©Ò¥H¤£»İ­n¬ö¿ı´¿¸g°Ñ³X, ¦]¬°¤@©w¤£·|­«½Æ½L­±
		}
		// ¦pªG¬O¨«¤l´N­nÀË¬d·|¤£·|­«½Æ
		else{
			cerr << "[*] Checking Repetition......\n";
			// FIXME: ·|¥d¦í¡H¡H
			// ¦pªG¤£¬OÂ½´Ñ¡A­n¦Ò¼{¬İ¬İ¦³¨S¦³¥i¯à³y¦¨ 3 ´`Àô
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15)); // FIN(15) ¤£·|¥Î¨ì
			record = this->visitedTable.getVal(nextB.hashKey);
			// ¦pªG flag ¤w¸g¬O 2 ¤F, ´N¤£­n¦A¨«³o¤@¨B, §ï¦¨¶ÃÂ½©Î¬O¶Ã¨«
			if(record.flag != nullptr && (*record.flag) >= 1){
				cerr << "[*] visit count: " << *record.flag << "\n";
				// ¦pªGÁÙ¦³¤l¥i¥HÂ½
				if(B.sumCnt > 0) {
					// Â½¤lµ´¹ï¤£·|³y¦¨¤T´`Àô
					assert(this->randomFlip(B, BestMove)); // ¤@©w·|¥i¥HÂ½
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove); // ¶Ã¨«, ¦ı¤£¯à¨«¨ì prohibit move
					assert(BestMove.st != BestMove.ed);
					
				}
			}

			// (BestMove ¦³¥i¯à¤w¸g´«¤F)¦pªG³Ì«á¿ï¥X¨Ó¬O¨«¦Y¨B, ´N°O¦í¤U¤@¨B°Ñ³X¦¸¼Æ, ¦pªG¬OÂ½¤l¨Bªº¸Ü´N¤£¥Î°O¤F¡A¦]¬°Â½¤l¤£·|³y¦¨´`Àô
			if(BestMove.st != BestMove.ed){
				nextB = B;
				nextB.DoMove(BestMove, FIN(15));
				record = this->visitedTable.getVal(nextB.hashKey);
				if(record.flag != nullptr){
					*record.flag += 1;
					assert(*record.flag < 32767); // À³¸Ó¤£¯à overflow
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
		// * who ÁÙ¨S¨M©w®É || B.sumCnt == 32
		if(B.who == -1 || B.sumCnt == 32) {
			assert(randomFlip(B, BestMove)); // ÀH«KÂ½
			return true;
		}
		// * ¦pªG¹ï¤è¯¥©Î¤ı¤w¸g¥X²{®É, ´Nª½±µÂ½¥L®ÇÃä(­n°÷¦h·t¤l®É¤~°µ³o¥ó¨Æ)
		assert(B.who != -1);
		const CLR oppColor = B.who^1;
		// ¦pªG§O¤H²Ä¤@¤lÂ½¥X ¤ı
		if((B.sumCnt == 31) && (B.cnt[FIN_C + oppColor*7] < 2 || B.cnt[FIN_K + oppColor*7] == 0)){
			assert(randomFlip(B, BestMove));
			return true;
		}
		
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
		CLR oppColor = (B.who == -1)? (-1):(B.who^1) ; // 
		// ±½¹L©Ò¦³ position
		for(POS p = 0; p < 32; p++){
			bool isDangerous = false;
			// ¦pªG³o­Ó¤l¤£¬O·t¤lªº¸Ü´N¸õ¹L
			if(B.fin[p] != FIN_X) continue;
			// ÀË¬d³o­Ó¤lªº¥|­±¤K¤è¦³¨S¦³¼Ä¤Hªº¤l
			for(int dir=0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				// ¦pªG¤£¬O¦X²zªº¨B´N¸õ¹L
				if(q == -1) continue;
				// ¦pªG¬O¼Ä¤èªº¤l, ºÉ¶q¤£­nÂ½®ÇÃä(°£¤F¯¥©Î«Ó), ¦pªG¬O unknown ÃC¦â¥Nªím¥i¥HÂ½
				if((oppColor != -1) && (GetColor(B.fin[q]) == oppColor)){
					isDangerous = true;
					// ¦pªG¬O¯¥ ©Î «Ó, mª½±µÂ½¥L¹j¾À, ½ä¥i¥H§â¥¦¨³³t¦Y±¼
					if((GetLevel(B.fin[q]) == LVL_C) || (GetLevel(B.fin[q]) == LVL_K)){
						BestMove = MOV(p, p);
						return true;
					}
				}
			}
			// ¦pªG¤£¦MÀI¤~ push back
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
			// ¦pªG¤£¬O¦Û¤vªº´Ñ, ´N¸õ¹L
			if(GetColor(pf) != B.who) continue;
			// ·j´M¥|­Ó¤è¦V
			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // ¦pªG¬O¤£¦Xªkªº¨B´N¸õ¹L
				FIN qf = B.fin[q];
				// ¬O¥i¥H¨«¦Yªº¨B ¥B ¤£¥i¥H¬O³Q¸T¤îªº¨B
				// ¦pªG¤£¬O¬¶¡A¥i¥Hª½±µ§PÂ_¯à¤£¯à¦Y¹j¾Àªº¤l
				if(GetLevel(pf) != LVL_C){
					if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
						movList.push_back(MOV(p, q)); // ¦s¤U±q p ©¹ q ¨«ªº move
					}
				}else{
					// ÀË¬d¬¶ªº¨«¦ì
					int countFinAlongDir = 0;
					if(qf == FIN_E) movList.push_back(MOV(p, q)); // ¦pªG¬OªÅ¤lªº¸Ü, ´N¥i¥H¨«
					// ©¹¤@­Óª½½u±½¹L¥h, ¦pªG qLocal ÁÙ¤£¬O -1 ¥B ¤¤¶¡ªº¤l¤´µM¤p©óµ¥©ó 1 Áû¤l¡A´N¥i¥HÄ~Äò
					for(POS qLocal = ADJ[q][dir]; (qLocal != -1) && (countFinAlongDir <= 1) ; 
						qLocal = ADJ[qLocal][dir]){
						const FIN qfLocal = B.fin[qLocal];
						// ¦pªG¤¤¶¡¹j¤@­Ó¤l ¦Ó¥B ·í«e qfLocal ¬O¹ï¤èÃC¦âªº¤l, «h¥i¥H¦Y¤l
						if(countFinAlongDir == 1 && GetColor(qfLocal) == (B.who^1) ){
							movList.push_back(MOV(p, qLocal));
							this->logger << "Cannon Move: (" << p << "," << qLocal << ")\n";
						}
						// ¦pªG²{¦b³o­Ó¤l¬O ÃC¦â¤l©Î·t¤l, ´N­p¤J countFinAlongDir
						if(qfLocal <= FIN_X){
							countFinAlongDir++;
						}
					}
				}
			}
		}
		
		if(movList.size() == 0){
			// Let it go ¥u¦nÅı¥L©M§½
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

	// ¥J²Ó«ä¦Ò
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		
		// ÀË¬d transposition table
		Record record; // ¦s retrieve ¥X¨Óªºµ²ªG
		// ¦pªG B.who ÁÙ¨S¨M©w´N¤£°µ
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
			// ²Ä¤@¼h¤£¥iª½±µ¦^¶Ç, ¤£µM·|§ä¤£¨ì BestMove
			if(depth != 0){
				// exact
				if(*record.flag == 0){
					// ÀË¬d²`«×: ©Ò³Ñ²`«× ¤ñ ¸Ì­±ªº©Ò³Ñ²`«×ÁÙ¤p
					if((this->cutOffDepth - depth) <= *record.depth){
						// ¥i¥Hª½±µ¦^¶Ç
						return *record.val;
					}
				}
				// lower bound(§â alpha ©Ô°ª¤@ÂI)
				else if(*record.flag == 1){
					if((this->cutOffDepth - depth) <= *record.depth){
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
		// 

		// * Time out
		// * If win (winner is not other)
		// * depth cut off
		// * no move can take
		timeval stop;
		gettimeofday(&stop, NULL);
		const int milliElapsed = (stop.tv_sec - start.tv_sec) * 1000 + (stop.tv_usec-start.tv_usec) / 1000;
		// ¦pªGÄ¹¤F´Nµ¹«Ü°ª¤À, »·¶W¹L©Ò¦³¦n¤lªº¤À¼Æ
		const CLR winner = B.getWinner();
		if(winner != -1){
			// Ä¹´Ñ®É±o¨ì ³Ì¤j ¤À¼Æ
			if(winner == B.who) return SearchEngine::vMax;
			// ¿é´Ñ®É±o¨ì ³Ì¤p ¤À¼Æ
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
		// FIXME:´ú¸Õ¤@¤U
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
				nextB.DoMove(lst.mov[i], FIN_E); // «á­± FIN_E ¥Î¤£¨ì
				// Scout
				t = -this->negaScout(nextB, -n, -MAX(alpha, m), BestMove, depth+1);
				if(depth == 0){
					
					cerr << "\n";
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
					// ¦pªG³o¤@¨B¬O§ğÀ»¨B(ed ¬O¹ï¤èÃC¦âªº¤l), ª½±µ¥[ 100 ¤À(100 ¤À
					// ¤£¶W¹L¤@­Ó§L¨òªº¤À¼Æ¡A¦]¦¹¶¶§Ç¤£·|ÅÜ¡A¦ı¬O­ì¥»¦P¼Ë¦nªº¨B·|¦]¦¹¤À¥X°ª¤U)
					#ifdef ATTACKMOVE
					// FIXME: BUG ¦]¬°¦pªG´`Àô½L­±ªº¸Ü, ¦]¬° transposition table caching ªºÃö«Y, ·|Åı³o¤@¨B attack move ¥[§¹ªº­È³Q¦s¦b table ¸Ì­±³y¦¨»~§P
					if(GetColor(B.fin[lst.mov[i].ed]) == (B.who^1)){
						m += 100;
						cerr << "[*] Attack Move!\n";
					}
					#endif
					BestMove = lst.mov[i];
				}
				
				if(m >= beta){
					// ¤w¸g¦³¦s¦b element ®É
					if(record.val != nullptr){
						// ©Ò³Ñ²`«×¤ñ¸û²`®É, §ó·s­È
						if((this->cutOffDepth - depth) >  *record.depth){
							*record.val = m;
							*record.flag = 1; // lower bound
							*record.depth = this->cutOffDepth - depth; // ¬ö¿ı©Ò³Ñ²`«×
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
				// ¦pªG¬O flag ¬O exact, ¨ºÀ³¸Ó­n¤@¼Ë
				if(*record.flag == 0 && (this->cutOffDepth - depth) == *record.depth){
					cerr << "What the hell?\n";
					cerr << "alpha: " << alpha << " ,beta: " << beta << "\n";
					cerr << "record.val: " << *record.val << " , now m: " << m << "\n";
					// TODO: «á¨Ó¹ê´úµo²{, ³oÃäÁÙ¬O·|¼Q, §ÚÄ±±o¥i¯à¬O¦]¬° lower bound 
					// ¸ò upper bound flag ¦³¾÷²v§â alpha beta bound ±À°ª(³o¬q function ªº³Ì«e­±), ¥H¦Ü©ó¦^¶Çªº­È¦³¥i¯à¸ò¥H«e¤£¦P
					// P.S. (³o¥u¬O§Úªº±À´ú, ¦pªG¦³±jÍª¾¹D¬°¤°»ò¦A³Â·Ğ§i¶D§Ú¤F)
					// assert(m == *record.val); // ¦pªG²`«×¤@¼Ë, ±o¨ìªº­È¥²¶·¤@¼Ë
					// ¨Ï¥Î³Ì·sªº­È!!!!!
					*record.val = m;
				}
				// ÀË¬d²`«×¦³¨S¦³ÅÜ²`
				// remain ²`«×¤j©ó hash entry ªº
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
		// ÁÙ¦³¨S¦³Â½¥X¨Óªº´Ñ
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
			// ¦pªG¨º­Ó´Ñ¤lm¤w¸gÂ½¶}¤F
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
					// ·|¤£·|¦³ overflow ªº°İÃD¡H
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

	// h(B): Nega version(return h(p), from the view of current Board)
	SCORE Eval(const BOARD &B) {
		assert(B.who != -1);
		SCORE cnt[2]={0,0}; // 0: ¬õ¤è, 1: ¶Â¤è
		// ­pºâÂ½¶}´Ñªº¤l¤O¤À¼Æ
		for(POS p=0;p<32;p++){
			const CLR c = GetColor(B.fin[p]);
			// ¦pªG¤£¬O¥¼Â½¶}ªº
			if(c != -1){
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
// ¤l¤O¤À¼Æ(0: «Ó, ..., 6:¨ò)
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200}; // http://www.csie.ntnu.edu.tw/~linss/Students_Thesis/2011_06_29_Lao_Yung_Hsiang.pdf
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore()+10000; // ³Óªº®É­ÔÀ³¸Ó­n¤ñ©Ò¦³¤À¼Æm°ª
SCORE SearchEngine::vMin = -SearchEngine::vMax;
#endif