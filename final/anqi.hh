#ifndef ANQI
#define ANQI
#include<random>

using namespace std;
// (color)
//  0 = ¬õ¤è (¤j¼g¦r¥À)
//  1 = ¶Â¤è (¤p¼g¦r¥À)
// -1 = m¤£¬O
typedef int CLR;
using ULL = unsigned long long;
// (level)
enum LVL {
	LVL_K=0, // «Ó±N King
	LVL_G=1, // ¥K¤h Guard
	LVL_M=2, // ¬Û¶H Minister
	LVL_R=3, // ÚÏ¨® Rook     // BIG5 ¨S¦³¤H³¡ªº¨®
	LVL_N=4, // ØX°¨ kNight
	LVL_C=5, // ¬¶¯¥ Cannon
	LVL_P=6  // §L¨ò Pawn
};

enum FIN {
	FIN_K= 0 /* 'K' «Ó */ , FIN_k= 7 /* 'k' ±N */ , FIN_X=14 /* 'X' ¥¼Â½ */ ,
	FIN_G= 1 /* 'G' ¥K */ , FIN_g= 8 /* 'g' ¤h */ , FIN_E=15 /* '-' ªÅ®æ */ ,
	FIN_M= 2 /* 'M' ¬Û */ , FIN_m= 9 /* 'm' ¶H */ ,
	FIN_R= 3 /* 'R' ÚÏ */ , FIN_r=10 /* 'r' ¨® */ ,
	FIN_N= 4 /* 'N' ØX */ , FIN_n=11 /* 'n' °¨ */ ,
	FIN_C= 5 /* 'C' ¬¶ */ , FIN_c=12 /* 'c' ¯¥ */ ,
	FIN_P= 6 /* 'P' §L */ , FIN_p=13 /* 'p' ¨ò */
};

// (position)
//  0  1  2  3
//  4  5  6  7
//  8  9 10 11
// 12 13 14 15
// 16 17 18 19
// 20 21 22 23
// 24 25 26 27
// 28 29 30 31
typedef int POS;

struct MOV {
	POS st; // şÂI
	POS ed; // ²×ÂI // ­Y ed==st ªí¥Ü¬OÂ½¤l
	bool isAttack;

	MOV() {}
	MOV(POS s,POS e):st(s),ed(e), isAttack(false){}
	MOV(POS s,POS e, bool attack):st(s),ed(e), isAttack(attack) {}

	bool operator==(const MOV &x) const {
		return (st==x.st && ed==x.ed && isAttack == x.isAttack);
	}
	MOV operator=(const MOV &x) {
		st=x.st;
		ed=x.ed;
		isAttack=x.isAttack;
		return MOV(x.st, x.ed, x.isAttack);
	}
};

struct MOVLST {
	int num;     // ¨«ªk¼Æ(²¾°Ê+¦Y¤l,¤£¥]¬AÂ½¤l)
	//MOV mov[68]; // ³Ì¦h¦³ 16 ­Ó´Ñ¤l¡A16 * 4(³Ì¦h²¾°Ê 4 ¨B) = 64, ¦]¦¹ upper bound ¬° 64 ¨B
	MOV mov[96]; // Â½¤l(³Ì¦h¥i¥HÂ½ 32 ­Ó) + ¨«¨B(¦Y¤l)(³Ì¦h¥i¥H¨« 16 * 4 = 64) = 96 ¨B
};

struct BOARD {
	CLR who;     // ²{¦b½ü¨ì¨º¤@¤è¤U
	FIN fin[32]; // ¦U­Ó¦ì¸m¤W­±Â\¤FÔ£
	int cnt[14]; // ¦UºØ´Ñ¤lªº¥¼Â½¶}¼Æ¶q
	int sumCnt; // ¦UºØ´Ñ¤lªº¥¼Â½¶}¼Æ¶qÁ`¼Æ
	unsigned long long hashKey; // ¦b Transposition Table ·|¥Î¤W
	// double evalScore; // ·í«e¼f§½¤À
	static ULL s[16][32]; // 64 bit
	static ULL color[3]; // 0:¬õ¤è, 1:¶Â¤è, 2:unknown
	static mt19937_64 gen;

	void NewGame();              // ¶}·s¹CÀ¸
	int  LoadGame(const char*);  // ¸ü¤J¹CÀ¸¨Ã¶Ç¦^®É­­(³æ¦ì:¬í)
	void Display() const;        // Åã¥Ü¨ì stderr ¤W
	int  MoveGen(MOVLST&) const; // ¦C¥X©Ò¦³¨«ªk (¨«¤l+¦Y¤l,¤£¥]¬AÂ½¤l)
	                             // ¦^¶Ç¨«ªk¼Æ¶q
	//
	int MoveGenWithFlip(MOVLST& ) const;       // ¦C¥X©Ò¦³¨«ªk (¨«¤l + ¦Y¤l + Â½¤l)
	//
	bool ChkLose() const;        // ÀË¬d·í«eª±®a(who)¬O§_¿é¤F
	CLR getWinner() const ;  // ¦^¶ÇÄ¹ªºª±®a

	bool ChkValid(MOV) const;    // ÀË¬d¬O§_¬°¦Xªk¨«ªk
	void Flip(POS,FIN=FIN_X);    // Â½¤l
	void Move(MOV);              // ²¾°Ê or ¦Y¤l
	void DoMove(MOV m, FIN f) ;
	static ULL hashDoMove(MOV m, FIN fromF, FIN toF);
	static bool initRandom(); 
	//void Init(int Board[32], int Piece[14], int Color);
	void Init(char Board[32], int Piece[14], int Color);
};

CLR  GetColor(FIN);    // ºâ¥X´Ñ¤lªºÃC¦â (0: ¬õ¤è, 1: ¶Â¤è, -1:m¤£¬O(¥i¯à¬O empty))
LVL  GetLevel(FIN);    // ºâ¥X´Ñ¤lªº¶¥¯Å
bool ChkEats(FIN,FIN); // §PÂ_²Ä¤@­Ó´Ñ¤l¯à§_¦Y²Ä¤G­Ó´Ñ¤l
void Output (MOV);     // ±Nµª®×¶Çµ¹ GUI



#endif
