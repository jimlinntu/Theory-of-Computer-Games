#ifndef TTB
#define TTB
#include<random>
#include<algorithm>
#include<iostream>
#include"anqi.hh"
using namespace std;
using ULL = unsigned long long;
using State = unsigned long long;
using Size_ = unsigned int;

// Transposition Table Record(point to tptable)
struct Record{
    double *val;
    short int *flag;
    int *depth;
    Record(){
        val = nullptr, flag = nullptr, depth = nullptr;
    };
    Record(double *value, short int *flag_, int *depth_): val(value), flag(flag_), depth(depth_){};
};

class TranspositionTable{
public:
    static const Size_ capacity = ((1U << 28)-1); //�i�H�˦h��(28 bit)    
    Size_ size; // ���h�� elements
    Size_ *head;
    Size_ *next;
    ULL *key; // array indices �O 0 ~ capacity-1
    double *val; // �s negaScout ��
    short int *flag; // 0: exact, 1: lower bound, 2: upper bound (�]��@ visit count)
    int *depth; // last search depth

    TranspositionTable();
    ~TranspositionTable();
    //
    void insert(ULL k, double v, short int f, int d);
    Record getVal(ULL key);
    // return updating hash ull
    static ULL hashDoMove(MOV m, FIN fromF, FIN toF);
    static bool initRandom();
};


#endif