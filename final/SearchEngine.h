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
	static SCORE finScore[7]; // 0: �ӱN, ..., 6: �L��
	int cutOffDepth = 5; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start;
	TranspositionTable transTable[2]; // 0: myself, 1: opponent 
	TranspositionTable visitedTable; // �ˬd���S���T�`���� table
	fstream logger;

	SearchEngine(){
		this->logger.open("log.txt",  ios::out | ios::trunc);
		this->logger << "Start Search" << "\n";
	}
	~SearchEngine(){
		this->logger.close();
	}

	// ���X�M�w
	MOV Play(const BOARD &B, int remain_milliseconds){
		MOV BestMove(-1, -1);
		// 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// �p�G���e�ѳX�L
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
		// * �V�h�t�l���ɭԪ�ɶ��֤@�I
		// this->timeOut = remain_milliseconds * ((33 - B.sumCnt) / 32) - 1;
		this->timeOut = 5000;
		
		gettimeofday(&this->start, NULL);
		cerr << "[*] Start negaScout" << "\n";
		SCORE negaScore= this->negaScout(B, SearchEngine::vMin, SearchEngine::vMax, BestMove, 0);
		cerr << "Negascore: " << negaScore << "\n";
		cerr << "[*] End negaScout" << "\n";
		
		// �p�G negaScout �j���X�B��
		if(BestMove.st == -1 || BestMove.ed == -1){
			assert(randomFlip(B, BestMove));
		}
		
		if(BestMove.st == BestMove.ed){
			// ½�Ѥl���|�y�� 3 �`��(�]��½�X�Ӥ���@�w�S��k���^��)
			// �ҥH���ݭn�������g�ѳX, �]���@�w���|���ƽL��
		}
		else{
			cerr << "[*] Checking Repetition......\n";
			// �p�G���O½�ѡA�n�Ҽ{�ݬݦ��S���i��y�� 3 �`��
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15));
			cerr << "[*] nextB.hashKey: " << bitset<64>(nextB.hashKey) << "\n";
			record = this->visitedTable.getVal(nextB.hashKey);
			// �p�G flag �w�g�O 2 �F, ���N�è�
			if(record.flag != nullptr) this->logger << "[*] This Board has been visited: " << *record.flag << "\n";
			if(record.flag != nullptr && (*record.flag) >= 2){
				if(B.sumCnt > 0) {
					// ½�l���藍�|�y���T�`��
					this->randomFlip(B, BestMove);
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove);
					assert(BestMove.st != BestMove.ed);
					
				}	
			}			
			// �p�G�̫��X�ӬO���Y�B, �N�O�@�U�ѳX�L
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
		// * who �٨S�M�w��
		if(B.who == -1) {
			assert(randomFlip(B, BestMove));
			return true;
		}
		// * �����m�٨S½��
		if(B.sumCnt == 32){
			// �o�B�@�w�|���� BestMove
			assert(randomFlip(B, BestMove));
			return true;
		}
		// * �p�G��该�w�g�X�{, �ӥB
		
		// * �p�G�Ӥw�g�X�{, �ӥB�ڪ��L��m�٦����Ӫ��ɭ�
		
		// * �i���W�ثe�m�O�O�H���l���ɭ�
		// * �i���W�ثe�m�٬O�ۤv���l���ɭ�
		return false;
	}
	
	bool randomFlip(const BOARD &B, MOV &BestMove){
		cerr << "randomFlip\n";
		// * ���n½��O�H���l����
		// * ���F�p�G�ĤH½�쯥�Ϋ�, ����½�L����F���L
		if(B.sumCnt==0) return false;
		vector<MOV> movList;
		CLR oppColor = (B.who^1); // �p�G B.who == -1, B.who^1 �|�ܦ� -2, ���v��
		
		for(POS p = 0; p < 32; p++){
			cerr << "p: "<< p << "\n";
			// �p�G�o�Ӥl�O�t�l
			bool isDangerous = false;
			if(B.fin[p] == FIN_X){
				//�ˬd�|���K�観�S���ĤH���l
				for(int dir=0; dir < 4; dir++){
					POS q = ADJ[p][dir];
					// �p�G�O�Ĥ誺�l, �ɶq���n½����(���F���Ϋ�)
					if(GetColor(B.fin[q]) == oppColor){
						isDangerous = true;
						// �p�G�O�� �� ��, �m����½�L�j��
						if(GetLevel(B.fin[q]) == LVL_C && GetLevel(B.fin[q]) == LVL_K){
							BestMove = MOV(p, p);
							return true;
						}
					}
				}
				// �p�G���M�I�~ push back
				if(!isDangerous) movList.push_back(MOV(p, p));
			}
		}
		assert(movList.size() > 0);
		uniform_int_distribution<int> U(0, movList.size()-1);
		BestMove = movList[U(gen)];
		cerr << "randflip end\n";
		return true;
	}
	void randomMove(const BOARD &B, MOV &BestMove, const MOV &prohitbitMove){
		vector<MOV> movList; // population
		for(POS p = 0; p < 32; p++){
			FIN pf = B.fin[p];
			// �ۤv���Ѥ~�~��
			if(GetColor(pf) != B.who) continue;

			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // �p�G�O���X�k���B�N���L
				FIN qf = B.fin[q];
				// �O�i�H���Y���B  �B ���i�H�O�Q�T��B
				if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
					movList.push_back(MOV(p, q));
					// (ChkEats �|�ˬd�O�_�O�ũι��l)�p�G���O��, �F���A��
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

	// �J�ӫ��
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		MOVLST lst;
		// �u�b root ���ɭ� �� �p�� 3 �l�S½�X�Ӫ��ɭ� �~����½�X�l�Ӫ����p, ��L���p�m�u��½�}�Ӫ��l���j�M
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
				nextB.DoMove(lst.mov[i], FIN_E); // �᭱ FIN_E �Τ���
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
		// �٦��S��½�X�Ӫ���
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
			// �p�G���ӴѤl�S���F
			if(B.cnt[i] == 0){
				continue;
			}
			// �p�G�o�ӧL���٦��s�b����
			else{
				nextB = B;
				// ���]�o�ӧL�سQ½�}��
				nextB.DoMove(mv, FIN(i));
				// 
				if(!isfirst){
					// (c / w1) * (alpha - vMax)
					tmpLocalRatio = ((double)B.sumCnt / (double)B.cnt[i]);
					A = tmpLocalRatio * (alpha - SearchEngine::vMax) + SearchEngine::vMax;
					B_ = tmpLocalRatio * (beta - SearchEngine::vMin) + SearchEngine::vMin;
					isfirst = false;
				}
				// TODO: �n�[�t���ܡH ��ı�o���ӭn�[�b�o��, �]�� [alpha, beta] �{�b�O�w��o�� node ��
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
		SCORE cnt[2]={0,0}; // 0: ����, 1: �¤�
		// �p��½�}�Ѫ��l�O����
		for(POS p=0;p<32;p++){
			const CLR c=GetColor(B.fin[p]);
			// �p�G���O��½�}��
			if(c!=-1){
				// �p��l�O����
				cnt[c] += SearchEngine::finScore[GetLevel(B.fin[p])];
			}
		}
		// �p�⥼½�}���l�O����
		for(int i=0;i<14;i++){
			cnt[i / 7] += (double)B.cnt[i] * SearchEngine::finScore[i % 7];
		}
		// MIN node: �V���N��� max node �V���Q
		// Max node: �V���N��� min node �V���Q
		// TODO: �Z���]�n��@�I���ơI�I�I
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
// �l�O����(0: ��, ..., 6:��)
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200};
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore();
SCORE SearchEngine::vMin = -SearchEngine::vMin;
#endif