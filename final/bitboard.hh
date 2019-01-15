#ifndef BITBOARD
#define BITBOARD

struct BOARD {
	CLR who;     // 現在輪到那一方下
	FIN fin[32]; // 各個位置上面擺了啥
	int cnt[14]; // 各種棋子的未翻開數量

	void NewGame();              // 開新遊戲
	int  LoadGame(const char*);  // 載入遊戲並傳回時限(單位:秒)
	void Display() const;        // 顯示到 stderr 上
	int  MoveGen(MOVLST&) const; // 列出所有走法(走子+吃子,不包括翻子)
	                             // 回傳走法數量
	bool ChkLose() const;        // 檢查當前玩家(who)是否輸了
	bool ChkValid(MOV) const;    // 檢查是否為合法走法
	void Flip(POS,FIN=FIN_X);    // 翻子
	void Move(MOV);              // 移動 or 吃子
	void DoMove(MOV m, FIN f) ;
	//void Init(int Board[32], int Piece[14], int Color);
	void Init(char Board[32], int Piece[14], int Color);
};

#endif