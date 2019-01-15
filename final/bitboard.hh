#ifndef BITBOARD
#define BITBOARD

struct BOARD {
	CLR who;     // �{�b���쨺�@��U
	FIN fin[32]; // �U�Ӧ�m�W���\�Fԣ
	int cnt[14]; // �U�شѤl����½�}�ƶq

	void NewGame();              // �}�s�C��
	int  LoadGame(const char*);  // ���J�C���öǦ^�ɭ�(���:��)
	void Display() const;        // ��ܨ� stderr �W
	int  MoveGen(MOVLST&) const; // �C�X�Ҧ����k(���l+�Y�l,���]�A½�l)
	                             // �^�Ǩ��k�ƶq
	bool ChkLose() const;        // �ˬd��e���a(who)�O�_��F
	bool ChkValid(MOV) const;    // �ˬd�O�_���X�k���k
	void Flip(POS,FIN=FIN_X);    // ½�l
	void Move(MOV);              // ���� or �Y�l
	void DoMove(MOV m, FIN f) ;
	//void Init(int Board[32], int Piece[14], int Color);
	void Init(char Board[32], int Piece[14], int Color);
};

#endif