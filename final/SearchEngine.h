#ifndef SearchEngine
#define SearchEngine
#include"anqi.hh"
#include<chrono>
#include<random>
#define MAX(a,b) (((a)>(b))?(a):(b))

using namespace std;

class SearchEngine{
public:
	using SCORE = int;
	static const SCORE INF=1000001;
	static const SCORE WIN=1000000;
	int cutOffDepth = 7; // search up to (depth == 7)
	random_device rd();
		
	//
	SCORE negaScout(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){
		MOVLST lst;
		B.MoveGenWithFlip(lst);
		// 1. depth cut off
		// 2. no move can take
		if(depth == this->cutOffDepth || lst.num == 0){
			return Eval(B, depth & 1);
		}

		SCORE m = -INF, n = beta;
		SCORE t;
		BOARD nextB;
		for(int i = 0; i < lst.num; i++){
			nextB = B;
			// chance node
			if(lst.mov[i].st == lst.mov[i].ed){
				t = -chanceSearch(B, alpha, beta, BestMove, depth);
			}
			// deterministic node
			else{
				t = -negaScout(B, -beta, -MAX(alpha, m), BestMove, depth+1);
			}
			// Take max
			if(t > m){
				if(n == beta || (this->cutOffDepth - depth) < 3 || t >= beta){
					m = t;
				}else{
					m = -negaScout(B, -beta, -t, BestMove, depth+1);
				}
				// record BestMove
				if(depth == 0){
					BestMove = lst.mov[i];
				}
				
				if(m >= beta){
					break;
				}

			}

			n = MAX(alpha, m) + 1; // null window
		}

		return m;
	}	

	SCORE chanceSearch(const BOARD &B, SCORE alpha, SCORE beta, MOV &BestMove, const int depth){

	}

	//
	SCORE Eval(const BOARD &B, bool isEven) {
		// 
		int cnt[2]={0,0};
		for(POS p=0;p<32;p++){const CLR c=GetColor(B.fin[p]);if(c!=-1)cnt[c]++;}
		for(int i=0;i<14;i++)cnt[GetColor(FIN(i))]+=B.cnt[i];
		
		return cnt[B.who]-cnt[B.who^1];
	}
    
};



#endif