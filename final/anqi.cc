/*****************************************************************************\
 * Theory of Computer Games: Fall 2013
 * Chinese Dark Chess Library by You-cheng Syu
 *
 * This file may not be used out of the class unless asking
 * for permission first.
 *
 * Modify by Hung-Jui Chang, December 2013
\*****************************************************************************/
#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<iostream>
#include<iterator>
#include<algorithm>
#include"anqi.hh"

#ifdef _WINDOWS
#include<windows.h>
#endif

ULL BOARD::s[16][32];
ULL BOARD::color[3];
mt19937_64 BOARD::gen(random_device{}());

static const char *tbl="KGMRNCPkgmrncpX-";

static const char *nam[16]={
	"帥","仕","相","硨","傌","炮","兵",
	"將","士","象","車","馬","砲","卒",
	"Ｏ","　"
};

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

// 如果是 FIN_X 或 FIN_E 都會是 -1
CLR GetColor(const FIN f) {
	return f<FIN_X?f/7:-1;
}

LVL GetLevel(const FIN f) {
	if(f >= FIN_X){
		throw runtime_error("GetLevel");
	}
	assert(f<FIN_X);
	return LVL(f%7);
}
// (小心 Bug!!) 這個僅能判斷子能不能互吃, 不管 position
bool ChkEats(FIN fa,FIN fb) {
	if(fa>=FIN_X)return false; // 如果 fa 是 FIN_X 或 FIN_E, 就直接不能走
	if(fb==FIN_X)return false; // 不考慮吃暗子的情形
	if(fb==FIN_E)return true ; // 如果這格是空的, 那就可以走(abuse ChkEats function)
	if(GetColor(fb)==GetColor(fa))return false; // 如果是同顏色的子 則不行移動

	const LVL la=GetLevel(fa);
	if(la==LVL_C)return true ; // 如果 a 子是炮, 則一定可以吃

	const LVL lb=GetLevel(fb);
	if(la==LVL_K)return lb!=LVL_P;
	if(la==LVL_P)return lb==LVL_P||lb==LVL_K;

	return la<=lb;
}

static void Output(FILE *fp,POS p) {
	fprintf(fp,"%c%d\n",'a'+p%4,8-(p/4));
}

void Output(MOV m) {
	FILE *fp=fopen("move.txt","w");
	assert(fp!=NULL);
	if(m.ed!=m.st) {
		fputs("0\n",fp);
		Output(fp,m.st);
		Output(fp,m.ed);
		fputs("0\n",fp);
	} else {
		fputs("1\n",fp);
		Output(fp,m.st);
		fputs("0\n",fp);
		fputs("0\n",fp);
	}
	fclose(fp);
}
// 根本沒用到的函數
void BOARD::NewGame() {
	static const int tbl[]={1,2,2,2,2,2,5};
	who=-1;
	for(POS p=0;p<32;p++)fin[p]=FIN_X;
	this->sumCnt = 0;
	for(int i=0;i<14;i++){
		cnt[i]=tbl[GetLevel(FIN(i))];
		this->sumCnt += this->cnt[i];
	}
}

static FIN find(char c) {
	return FIN(strchr(tbl,c)-tbl);
}

static POS LoadGameConv(const char *s) {
	return (8-(s[1]-'0'))*4+(s[0]-'a');
}

static void LoadGameReplay(BOARD &brd,const char *cmd) {
	if(cmd[2]!='-')brd.Flip(LoadGameConv(cmd),find(cmd[3]));
	else brd.Move(MOV(LoadGameConv(cmd),LoadGameConv(cmd+3)));
}

static POS mkpos(int x,int y) {
	return x*4+y;
}

// void BOARD::Init(int Board[32], int Piece[14], int Color) {
//     for (int i = 0 ; i < 14; ++i) {
// 	cnt[i] = Piece[i];
//     }
//     for (int i = 0 ; i < 32; ++i) {
// 	switch(Board[i]) {
// 	    case  0: fin[i] = FIN_E;break;
// 	    case  1: fin[i] = FIN_K;cnt[FIN_K]--;break;
// 	    case  2: fin[i] = FIN_G;cnt[FIN_G]--;break;
// 	    case  3: fin[i] = FIN_M;cnt[FIN_M]--;break;
// 	    case  4: fin[i] = FIN_R;cnt[FIN_R]--;break;
// 	    case  5: fin[i] = FIN_N;cnt[FIN_N]--;break;
// 	    case  6: fin[i] = FIN_C;cnt[FIN_C]--;break;
// 	    case  7: fin[i] = FIN_P;cnt[FIN_P]--;break;
// 	    case  8: fin[i] = FIN_X;break;
// 	    case  9: fin[i] = FIN_k;cnt[FIN_k]--;break;
// 	    case 10: fin[i] = FIN_g;cnt[FIN_g]--;break;
// 	    case 11: fin[i] = FIN_m;cnt[FIN_m]--;break;
// 	    case 12: fin[i] = FIN_r;cnt[FIN_r]--;break;
// 	    case 13: fin[i] = FIN_n;cnt[FIN_n]--;break;
// 	    case 14: fin[i] = FIN_c;cnt[FIN_c]--;break;
// 	    case 15: fin[i] = FIN_p;cnt[FIN_p]--;break;
// 	}
//     }
//     who = Color;
// }

void BOARD::Init(char Board[32], int Piece[14], int Color) {
	this->sumCnt = 0;
	this->hashKey = 0ULL;
    for (int i = 0; i < 14; ++i) {
		this->cnt[i] = Piece[i];
		sumCnt += this->cnt[i];
		cerr << "[*] sumCnt: " << sumCnt << "\n";
    }
	// 把隨機值設好
	BOARD::initRandom(); 
	// 輸入 board from protocol
    for (int i = 0 ; i < 32; ++i) {
		int p = (7-i/4)*4+i%4;
		// 
		switch(Board[i]) {
			case '-': fin[p] = FIN_E;break;
			case 'K': fin[p] = FIN_K;cnt[FIN_K]--;sumCnt--;break;
			case 'G': fin[p] = FIN_G;cnt[FIN_G]--;sumCnt--;break;
			case 'M': fin[p] = FIN_M;cnt[FIN_M]--;sumCnt--;break;
			case 'R': fin[p] = FIN_R;cnt[FIN_R]--;sumCnt--;break;
			case 'N': fin[p] = FIN_N;cnt[FIN_N]--;sumCnt--;break;
			case 'C': fin[p] = FIN_C;cnt[FIN_C]--;sumCnt--;break;
			case 'P': fin[p] = FIN_P;cnt[FIN_P]--;sumCnt--;break;
			case 'X': fin[p] = FIN_X;break;
			case 'k': fin[p] = FIN_k;cnt[FIN_k]--;sumCnt--;break;
			case 'g': fin[p] = FIN_g;cnt[FIN_g]--;sumCnt--;break;
			case 'm': fin[p] = FIN_m;cnt[FIN_m]--;sumCnt--;break;
			case 'r': fin[p] = FIN_r;cnt[FIN_r]--;sumCnt--;break;
			case 'n': fin[p] = FIN_n;cnt[FIN_n]--;sumCnt--;break;
			case 'c': fin[p] = FIN_c;cnt[FIN_c]--;sumCnt--;break;
			case 'p': fin[p] = FIN_p;cnt[FIN_p]--;sumCnt--;break;
		}
		assert(tbl[(int)fin[p]] == Board[i]);
		// update hashKey
		this->hashKey ^= this->s[(int)fin[p]][p]; //把每個子的 hash 值 xor 進去
    }
    who = Color;
	// WARNING: 我們在這裡先不把 turn 的資訊加進去, 之後 DoMove 會做
	assert(this->sumCnt >= 0); // 未翻的子不可以小於 0
}

int BOARD::LoadGame(const char *fn) {
	FILE *fp=fopen(fn,"r");
	assert(fp!=NULL);

	while(fgetc(fp)!='\n');

	while(fgetc(fp)!='\n');

	fscanf(fp," %*c");
	for(int i=0;i<14;i++)fscanf(fp,"%d",cnt+i);

	for(int i=0;i<8;i++) {
		fscanf(fp," %*c");
		for(int j=0;j<4;j++) {
			char c;
			fscanf(fp," %c",&c);
			fin[mkpos(i,j)]=find(c);
		}
	}

	int r = 0; // 預設是紅色
	fscanf(fp," %*c%*s%d" ,&r);
	who=(r==0||r==1?r:-1);
	fscanf(fp," %*c%*s%d ",&r);

	for(char buf[64];fgets(buf,sizeof(buf),fp);) {
		if(buf[2]<'0'||buf[2]>'9')break;
		char xxx[16],yyy[16];
		const int n=sscanf(buf+2,"%*s%s%s",xxx,yyy);
		if(n>=1)LoadGameReplay(*this,xxx);
		if(n>=2)LoadGameReplay(*this,yyy);
	}
	//
	cerr << "Color is: " << r << "\n";
	cerr << "Who is: " << this->who << "\n";
	this->sumCnt = 0;
	for(int i = 0; i < 14; i++){
		this->sumCnt += this->cnt[i];
	}
	this->initRandom();
	this->hashKey = 0ULL;
	this->hashKey ^= BOARD::color[r];
	for(int i = 0; i < 32; i++){
		this->hashKey ^= BOARD::s[(int)this->fin[i]][i];
	}
	fclose(fp);
	return r;
}

void BOARD::Display() const {
#ifdef _WINDOWS
	HANDLE hErr=GetStdHandle(STD_ERROR_HANDLE);
#endif
	for(int i=0;i<8;i++) {
#ifdef _WINDOWS
		SetConsoleTextAttribute(hErr,8);
#endif
		for(int j=0;j<4;j++)fprintf(stderr,"[%02d]",mkpos(i,j));
		if(i==2) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,12);
#endif
			fputs("  ",stderr);
			for(int j=0;j<7;j++)for(int k=0;k<cnt[j];k++)fputs(nam[j],stderr);
		}
		fputc('\n',stderr);
		for(int j=0;j<4;j++) {
			const FIN f=fin[mkpos(i,j)];
			const CLR c=GetColor(f);
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,(c!=-1?12-c*2:7));
#endif
			fprintf(stderr," %s ",nam[fin[mkpos(i,j)]]);
		}
		if(i==0) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,7);
#endif
			fputs("  輪到 ",stderr);
			if(who==0) {
#ifdef _WINDOWS
				SetConsoleTextAttribute(hErr,12);
#endif
				fputs("紅方",stderr);
			} else if(who==1) {
#ifdef _WINDOWS
				SetConsoleTextAttribute(hErr,10);
#endif
				fputs("黑方",stderr);
			} else {
				fputs("？？",stderr);
			}
		} else if(i==1) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,7);
#endif
			fputs("  尚未翻出：",stderr);
		} else if(i==2) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,10);
#endif
			fputs("  ",stderr);
			for(int j=7;j<14;j++)for(int k=0;k<cnt[j];k++)fputs(nam[j],stderr);
		}
		fputc('\n',stderr);
	}
#ifdef _WINDOWS
	SetConsoleTextAttribute(hErr,7);
#endif
}

int BOARD::MoveGen(MOVLST &lst) const {
	static POS order[32] = {9, 10, 13, 14, 17, 18, 21, 22, 5, 6, 25, 26, 4, 7, 8, 11, 24, 27, 20, 23,
							12, 15, 16, 19, 1, 2, 29, 30, 0, 3, 28, 31};
	// TODO: 要先從離敵人進的子開始搜!!!!!
	if(who==-1)return false;
	lst.num=0;
	for(POS i=0;i<32;i++) {
		const POS p = order[i];
		const FIN pf=fin[p];
		if(GetColor(pf)!=who)continue;
		const LVL pl=GetLevel(pf);
		for(int z=0;z<4;z++) {
			const POS q=ADJ[p][z];
			if(q==-1)continue;
			const FIN qf=fin[q];
			if(pl!=LVL_C){if(!ChkEats(pf,qf))continue;}
			else if(qf!=FIN_E)continue;
			// 後面那串是指是否為 attack move
			lst.mov[lst.num++]=MOV(p,q, GetColor(qf) == (this->who^1));
		}
		if(pl!=LVL_C)continue;
		for(int z=0;z<4;z++) {
			int c=0;
			for(POS q=p;(q=ADJ[q][z])!=-1;) {
				const FIN qf=fin[q];
				if(qf==FIN_E||++c!=2)continue;
				if(qf!=FIN_X&&GetColor(qf)!=who){
					lst.mov[lst.num++]=MOV(p,q, GetColor(qf) == (this->who^1));			
				}
				break;
			}
		}
	}
	// 隨機大法, 以避免吃不到子
	// if(lst.num > 0) shuffle(begin(lst.mov), begin(lst.mov) + lst.num, BOARD::gen);
	// 從 攻擊步開始先搜(這樣的話如果 tie 也可以優先選擇攻擊步)
	sort(begin(lst.mov), begin(lst.mov) + lst.num, 
		[](const MOV &left, const MOV &right)-> bool{
			return ((int)left.isAttack > (int)right.isAttack); // 如果是 attack 的話就算比較 "小" 的值
		});
	// for(int i = 0; i < lst.num; i++){
	// 	cerr << ((lst.mov[i].isAttack)? ('t'):('f')) << " ";
	// }
	// cerr << "\n";
	return lst.num;
}

bool BOARD::ChkLose() const {
	if(who==-1)return false;

	bool fDark=false; //是否還有沒有翻開的子
	for(int i=0;i<14;i++) {
		if(cnt[i]==0)continue;
		if(GetColor(FIN(i))==who)return false; // 如果有找到自己的棋子，就還沒輸
		fDark=true;
	}

	bool fLive=false; // 是否還有活著的子
	for(POS p=0;p<32;p++)if(GetColor(fin[p])==who){fLive=true;break;}
	if(!fLive)return true;

	MOVLST lst;
	return !fDark&&MoveGen(lst)==0;
}

CLR BOARD::getWinner() const{
	if(who == -1) return -1; // 根本還沒開局翻棋
	bool redfDark = false, blackfDark = false; // whether red/black has dark chess
	CLR tmpClr;
	// * 檢查14種兵種沒翻的棋
	for(int i = 0; i < 14; i++){
		// 如果這個棋子已經沒有了, 就繼續往下一個兵種找找看
		if(this->cnt[i] == 0) continue;
		tmpClr = GetColor(FIN(i));
		// if red still contain a dark chess
		if(tmpClr == 0){
			redfDark = true; // 還有紅色棋
		}else if(tmpClr == 1){
			blackfDark = true; // 還有黑色棋
		}else continue;
		// 如果兩個棋都還有的話，就可以跳掉了, 其他情形我們還不能確定
		if(redfDark && blackfDark){
			return -1;
		}
	}
	bool redLive = false, blackLive = false;
	// * 檢查還活著的棋
	for(POS p = 0; p < 32; p++){
		tmpClr = GetColor(this->fin[p]);
		if(tmpClr == 0){
			redLive = true;
		}else if(tmpClr == 1){
			blackLive = true;
		}else continue;
		// 如果檯面上兩方的棋分別都存在, 就確定沒有贏家了
		if(redLive && blackLive){
			return -1; // 還沒有贏家
		}
	}
	// 把蓋著的棋和開著的棋合起來一起看
	redLive = redfDark || redLive; //紅色活著
	blackLive = blackfDark || blackLive; // 黑色活著
	// 兩個都活著
	if(redLive && blackLive){ 
		return -1;
	}else if(redLive){
		return 0; // 紅贏
	}else{
		return 1; // 黑贏
	}
}

bool BOARD::ChkValid(MOV m) const {
	if(m.ed!=m.st) {
		MOVLST lst;
		MoveGen(lst);
		for(int i=0;i<lst.num;i++)if(m==lst.mov[i])return true;
	} else {
		if(m.st<0||m.st>=32)return false;
		if(fin[m.st]!=FIN_X)return false;
		for(int i=0;i<14;i++)if(cnt[i]!=0)return true;
	}
	return false;
}

void BOARD::Flip(POS p,FIN f) {
	// FIXME: 如果 f 是未翻狀態, 則我們就亂翻一個假裝他是那個棋子
	if(f==FIN_X) {
		assert(false); // deprecated
		int i,sum=0;
		for(i=0;i<14;i++)    sum+=cnt[i];
		sum=rand()%sum; // random 
		for(i=0;i<14;i++)if((sum-=cnt[i])<0)break;
		f=FIN(i);
	}
	fin[p]=f; // 從 Judge 那邊拿來的翻子 或 自己亂掰的翻子
	cnt[f]--; assert(cnt[f] >= 0);// 減少該子翻出來的數量
	sumCnt--; assert(sumCnt >= 0);// 減少未翻出子的數量
	if(who==-1) who=GetColor(f);
	who^=1;
}

void BOARD::Move(MOV m) {
	// deprecated
	assert(false);
	if(m.ed!=m.st) {
		fin[m.ed]=fin[m.st];
		fin[m.st]=FIN_E;
		who^=1;
	} else {
		// FIXME: fake flip
		Flip(m.st);
	}
}
// f 會是實際翻出來的子(從 protocol 傳來的)
void BOARD::DoMove(MOV m, FIN f) {
	static bool isFirst = true; // 第一次的時候要把 who 的資訊 xor 到 hashKey 裡面
	FIN fromF=fin[m.st], toF=fin[m.ed];
	if(m.st == m.ed){
		assert(m.st != -1);
		//FIXME: ASSERTION Fail???
		assert(fromF == FIN_X);
		toF = f; // 如果是翻棋步, 那 toF 就會是翻出來的棋
	}
	// update hash key (因為 who 會在 main.cc 中先設好)
	// 如果是第一次被 run 到, 在上面先補上自己原本 color 的 hash, 如果不是的話 自己的 color hash 已經在裡面了
	if(isFirst){
		assert(this->who != -1);
		this->hashKey ^= BOARD::color[this->who]; //
		isFirst = false; // 關掉
	}
	// 更新 hashkey
	this->hashKey ^= BOARD::hashDoMove(m, fromF, toF);
	// 假設下出 走步或吃子
    if (m.ed!=m.st) {
		fin[m.ed]=fin[m.st];
		fin[m.st]=FIN_E;
		who^=1;
    }
	// 假設下出 翻子
    else {
		Flip(m.st, f);
    }
}
// return updating hash ull
ULL BOARD::hashDoMove(MOV m, FIN fromF, FIN toF){
    // 動作一律都是: 清乾淨原本的 -> 把新東西放上去
    assert(m.st != -1 && m.ed != -1);
    ULL updateHash = 0ULL;
    // * 換 turn
    updateHash ^= color[0] ^ color[1];
    if(m.st == m.ed){
		assert(fromF == FIN_X);
		assert((int)toF < 14);
        // * A flip
        // 清掉 暗棋 放上 新棋
        updateHash ^= s[FIN_X][m.st] ^ s[toF][m.ed]; 
    }else{
		assert(fromF != FIN_X && toF != FIN_X); // 移動子不可為 FIN_X, 目標子一定是空的或是可以吃的子(不會是暗子)
        // * change place
        // 把原本的棋子先清掉換成空白, 再把原本的棋子放到新的地方去 (toF 有可能是空白或是敵子)
        updateHash ^= (s[fromF][m.st] ^ s[FIN_E][m.st]) ^ (s[toF][m.ed] ^ s[fromF][m.ed]); 
    }
    return updateHash;
}
bool BOARD::initRandom(){
    // Random a 64 bit number
    uniform_int_distribution<ULL> U(0, (0ULL - 1ULL));
    
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 32; j++){
            s[i][j] = U(BOARD::gen);
        }
    }
    color[0] = U(BOARD::gen); // red
    color[1] = U(BOARD::gen); // black
    color[2] = U(BOARD::gen); // 都不是
	return true;
}

// TODO:
/*
int BOARD::MoveGenWithFlip(MOVLST &lst) const{
	// TODO: Sort move ordering (先考慮 level 大的棋子)
	lst.num=0;
	// 先考慮中間的子

	// 掃過 32 個位置
	for(POS p=0;p<32;p++) {
		const FIN pf=fin[p];
		CLR finColor = GetColor(pf);
		// If 'pf' is FIN_X
		if(pf == FIN_X){
			lst.mov[lst.num++] = MOV(p, p); // flip move 翻子步
		}
		// 只檢查自己的棋, 如果不是自己的棋, 那就不會列入 move list
		else if(finColor == this->who){
			const LVL pl=GetLevel(pf); // 判斷棋種
			// 測試 4 個方向能不能走
			for(int z=0;z<4;z++) {
				const POS q=ADJ[p][z]; 
				if(q==-1)continue; // 如果某個方向是違法的, 就跳過
				const FIN qf=fin[q]; // 檢查你要走到的地方現在是什麼棋子
				if(pl!=LVL_C){if(!ChkEats(pf,qf))continue;} // 如果不是砲才檢查, 如果發現不能吃(或移動), 就跳過
				else if(qf!=FIN_E)continue; // 如果是砲的話, 那就只有 FIN_E 的時候才能走
				lst.mov[lst.num++]=MOV(p,q);
			}
			// 不是砲就跳過
			if(pl!=LVL_C)continue;
			// 特別檢查砲的吃子
			for(int z=0;z<4;z++) {
				int c=0; // 記錄這個方向現在的子數
				// q 設在自己這個點, 往 z 方向掃過去看看有沒有可以吃個子
				for(POS q=p;(q=ADJ[q][z])!=-1;) {
					const FIN qf=fin[q];
					if(qf==FIN_E||++c!=2)continue; 
					// 如果不是 暗子 且 不是自己的子, 就可以設為移動步
					if(qf!=FIN_X&&GetColor(qf)!=who)lst.mov[lst.num++]=MOV(p,q);
					break;
				}
			}
		}
	}
	return lst.num;
}
*/