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
#define ATTACKMOVE


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
	int cutOffDepth = 7; // search up to (depth == 7)
	int timeOut; // in milliseconds
	timeval start;
	TranspositionTable transTable[2]; // 0: red, 1: black
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
		// Retrieve from visitedTable 
		Record record = this->visitedTable.getVal(B.hashKey);
		if(record.flag != nullptr){
			// �p�G���e�ѳX�L
			*record.flag += 1; // add visit time
		}else{
			this->visitedTable.insert(B.hashKey, -1, 1, -1);
		}
		// �}�� Rule
		if(this->specialBoardCondition(B, BestMove)){
			return BestMove;
		}
		assert(B.who != -1); // B.who == -1 �|�Q special condition �ױ�
		// Pre-evaluation
		SCORE preScore = this->Eval(B);
		this->logger << "Prescore is: " << preScore << "\n" << flush;
		// NegaMax 
		// * �V�h�t�l���ɭԪ�ɶ��֤@�I
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
		
		// �p�G�]���S�� MOVLIST, �ӨS���B����, ��½
		if(BestMove.st == -1 || BestMove.ed == -1){
			// �p�G�S�����ʨB���ܤ@�w�O���l�i�H½ (���D�d�� ex. �Ⱖ���d���L, ���O�o�ӷ|���Q Judge �P�w����)
			assert(randomFlip(B, BestMove));
		}
		
		if(BestMove.st == BestMove.ed){
			// ½�Ѥl���|�y�� 3 �`��(�]��½�X�Ӥ���@�w�S��k���^��)
			// �ҥH���ݭn�������g�ѳX, �]���@�w���|���ƽL��
		}
		// �p�G�O���l�N�n�ˬd�|���|����
		else{
			cerr << "[*] Checking Repetition......\n";
			// FIXME: �|�d��H�H
			// �p�G���O½�ѡA�n�Ҽ{�ݬݦ��S���i��y�� 3 �`��
			BOARD nextB = B;
			nextB.DoMove(BestMove, FIN(15)); // FIN(15) ���|�Ψ�
			record = this->visitedTable.getVal(nextB.hashKey);
			// �p�G flag �w�g�O 2 �F, �N���n�A���o�@�B, �令��½�άO�è�
			if(record.flag != nullptr && (*record.flag) >= 1){
				cerr << "[*] visit count: " << *record.flag << "\n";
				// �p�G�٦��l�i�H½
				if(B.sumCnt > 0) {
					// ½�l���藍�|�y���T�`��
					assert(this->randomFlip(B, BestMove)); // �@�w�|�i�H½
				}else{
					MOV prohibitMove = BestMove;
					this->randomMove(B, BestMove, prohibitMove); // �è�, �����ਫ�� prohibit move
					assert(BestMove.st != BestMove.ed);
					
				}
			}

			// (BestMove ���i��w�g���F)�p�G�̫��X�ӬO���Y�B, �N�O��U�@�B�ѳX����, �p�G�O½�l�B���ܴN���ΰO�F�A�]��½�l���|�y���`��
			if(BestMove.st != BestMove.ed){
				nextB = B;
				nextB.DoMove(BestMove, FIN(15));
				record = this->visitedTable.getVal(nextB.hashKey);
				if(record.flag != nullptr){
					*record.flag += 1;
					assert(*record.flag < 32767); // ���Ӥ��� overflow
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
		// * who �٨S�M�w�� || B.sumCnt == 32
		if(B.who == -1 || B.sumCnt == 32) {
			assert(randomFlip(B, BestMove)); // �H�K½
			return true;
		}
		// * �p�G��该�Τ��w�g�X�{��, �N����½�L����(�n���h�t�l�ɤ~���o���)
		assert(B.who != -1);
		const CLR oppColor = B.who^1;
		if((B.sumCnt > 28) && (B.cnt[FIN_C + oppColor*7] < 2 || B.cnt[FIN_K + oppColor*7] == 0)){
			assert(randomFlip(B, BestMove));
			return true;
		}
		
		// * �i���W�ثe�m�O�O�H���l���ɭ�
		// * �i���W�ثe�m�٬O�ۤv���l���ɭ�
		return false;
	}
	
	bool randomFlip(const BOARD &B, MOV &BestMove){
		// * ���n½��O�H���l����
		// * ���F�p�G�ĤH½�쯥�Ϋ�, ����½�L����F���L
		if(B.sumCnt==0) return false;
		vector<MOV> movList;
		vector<MOV> dangerList;
		CLR oppColor = (B.who == -1)? (-1):(B.who^1) ; // 
		// ���L�Ҧ� position
		for(POS p = 0; p < 32; p++){
			bool isDangerous = false;
			// �p�G�o�Ӥl���O�t�l���ܴN���L
			if(B.fin[p] != FIN_X) continue;
			// �ˬd�o�Ӥl���|���K�観�S���ĤH���l
			for(int dir=0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				// �p�G���O�X�z���B�N���L
				if(q == -1) continue;
				// �p�G�O�Ĥ誺�l, �ɶq���n½����(���F���Ϋ�), �p�G�O unknown �C��N��m�i�H½
				if((oppColor != -1) && (GetColor(B.fin[q]) == oppColor)){
					isDangerous = true;
					// �p�G�O�� �� ��, �m����½�L�j��, ��i�H�⥦���t�Y��
					if((GetLevel(B.fin[q]) == LVL_C) || (GetLevel(B.fin[q]) == LVL_K)){
						BestMove = MOV(p, p);
						return true;
					}
				}
			}
			// �p�G���M�I�~ push back
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
			// �p�G���O�ۤv����, �N���L
			if(GetColor(pf) != B.who) continue;
			// �j�M�|�Ӥ�V
			for(int dir = 0; dir < 4; dir++){
				POS q = ADJ[p][dir];
				if(q == -1) continue; // �p�G�O���X�k���B�N���L
				FIN qf = B.fin[q];
				// �O�i�H���Y���B �B ���i�H�O�Q�T��B
				// �p�G���O���A�i�H�����P�_�ण��Y�j�����l
				if(GetLevel(pf) != LVL_C){
					if(ChkEats(pf, qf) && !(p == prohitbitMove.st && q == prohitbitMove.ed)){
						movList.push_back(MOV(p, q)); // �s�U�q p �� q ���� move
					}
				}else{
					// �ˬd��������
					int countFinAlongDir = 0;
					if(qf == FIN_E) movList.push_back(MOV(p, q)); // �p�G�O�Ťl����, �N�i�H��
					// ���@�Ӫ��u���L�h, �p�G qLocal �٤��O -1 �B �������l���M�p�󵥩� 1 ���l�A�N�i�H�~��
					for(POS qLocal = ADJ[q][dir]; (qLocal != -1) && (countFinAlongDir <= 1) ; 
						qLocal = ADJ[qLocal][dir]){
						const FIN qfLocal = B.fin[qLocal];
						// �p�G�����j�@�Ӥl �ӥB ��e qfLocal �O����C�⪺�l, �h�i�H�Y�l
						if(countFinAlongDir == 1 && GetColor(qfLocal) == (B.who^1) ){
							movList.push_back(MOV(p, qLocal));
							this->logger << "Cannon Move: (" << p << "," << qLocal << ")\n";
						}
						// �p�G�{�b�o�Ӥl�O �C��l�ηt�l, �N�p�J countFinAlongDir
						if(qfLocal <= FIN_X){
							countFinAlongDir++;
						}
					}
				}
			}
		}
		
		if(movList.size() == 0){
			// Let it go �u�n���L�M��
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

	// �J�ӫ��
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		
		// �ˬd transposition table
		Record record; // �s retrieve �X�Ӫ����G
		// �p�G B.who �٨S�M�w�N����
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
			// �Ĥ@�h���i�����^��, ���M�|�䤣�� BestMove
			if(depth != 0){
				// exact
				if(*record.flag == 0){
					// �ˬd�`��: �ҳѲ`�� �� �̭����ҳѲ`���٤p
					if((this->cutOffDepth - depth) <= *record.depth){
						// �i�H�����^��
						return *record.val;
					}
				}
				// lower bound(�� alpha �԰��@�I)
				else if(*record.flag == 1){
					if((this->cutOffDepth - depth) <= *record.depth){
						alpha = MAX(alpha, *record.val);
					}
				}
				// upper bound(�� beta �ԧC�@�I)
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
		// �u�b root ���ɭ� �� �p�� 3 �l�S½�X�Ӫ��ɭ� �~����½�X�l�Ӫ����p, ��L���p�m�u��½�}�Ӫ��l���j�M
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
		// �p�GĹ�F�N���ܰ���, ���W�L�Ҧ��n�l������
		const CLR winner = B.getWinner();
		if(winner != -1){
			// Ĺ�Ѯɱo�� �̤j ����
			if(winner == B.who) return SearchEngine::vMax;
			// ��Ѯɱo�� �̤p ����
			return SearchEngine::vMin;
		}
		if(milliElapsed > this->timeOut || depth == this->cutOffDepth 
			|| lst.num == 0){
			this->logger << "[*] Scout Depth: " << depth << "\n";
			this->logger << "[*] Board: " << B.who << "\n";
			this->logger << "[*] Elapse seconds: " << milliElapsed / 1000 << "\n";
			return Eval(B);
		}
		// FIXME:���դ@�U
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
				nextB.DoMove(lst.mov[i], FIN_E); // �᭱ FIN_E �Τ���
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
					// �p�G�o�@�B�O�����B(ed �O����C�⪺�l), �����[ 100 ��(100 ��
					// ���W�L�@�ӧL�򪺤��ơA�]�����Ǥ��|�ܡA���O�쥻�P�˦n���B�|�]�����X���U)
					#ifdef ATTACKMOVE
					if(GetColor(B.fin[lst.mov[i].ed]) == (B.who^1)){
						m += 100;
						cerr << "[*] Attack Move!\n";
					}
					#endif
					BestMove = lst.mov[i];
				}
				
				if(m >= beta){
					// �w�g���s�b element ��
					if(record.val != nullptr){
						// �ҳѲ`�פ���`��, ��s��
						if((this->cutOffDepth - depth) >  *record.depth){
							*record.val = m;
							*record.flag = 1; // lower bound
							*record.depth = this->cutOffDepth - depth; // �����ҳѲ`��
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
			// element �s�b��
			if(record.val != nullptr){
				// �ˬd�`�צ��S���ܲ`
				// remain �`�פj�� hash entry ��
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
		// �٦��S��½�X�Ӫ���
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
			// �p�G���ӴѤl�m�w�g½�}�F
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
					// �|���|�� overflow �����D�H
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

	// h(B): Nega version(return h(p), from the view of current Board)
	SCORE Eval(const BOARD &B) {
		assert(B.who != -1);
		SCORE cnt[2]={0,0}; // 0: ����, 1: �¤�
		// �p��½�}�Ѫ��l�O����
		for(POS p=0;p<32;p++){
			const CLR c = GetColor(B.fin[p]);
			// �p�G���O��½�}��
			if(c != -1){
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
// �l�O����(0: ��, ..., 6:��)
SCORE SearchEngine::finScore[7] = {6095, 3047, 1523, 761, 380, 420, 200}; // http://www.csie.ntnu.edu.tw/~linss/Students_Thesis/2011_06_29_Lao_Yung_Hsiang.pdf
SCORE SearchEngine::INF=numeric_limits<SCORE>::max();
SCORE SearchEngine::NINF=numeric_limits<SCORE>::lowest();
SCORE SearchEngine::vMax = SearchEngine::sumOfFinScore()+10000; // �Ӫ��ɭ����ӭn��Ҧ����Ɛm��
SCORE SearchEngine::vMin = -SearchEngine::vMax;
#endif