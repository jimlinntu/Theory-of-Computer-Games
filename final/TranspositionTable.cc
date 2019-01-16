#include"TranspositionTable.hh"
#include<bitset>
#include<assert.h>
// �ˬd����
TranspositionTable::TranspositionTable(){
    this->size = 0;
    this->head = new Size_[TranspositionTable::capacity];
    this->next = new Size_[TranspositionTable::capacity];
    this->key = new ULL[TranspositionTable::capacity];
    this->val = new double[TranspositionTable::capacity];
    this->flag = new short int[TranspositionTable::capacity];
    this->depth = new int[TranspositionTable::capacity];
    
    // ��J 32 bit ���� 1 ���Ʀr(����� negation �����K)
    fill(this->head, this->head+capacity, (0U-1U));
    cerr << "head[0]" << bitset<32>(this->head[0]) << "\n";
    assert(this->head[0] == 4294967295U && this->head[capacity-1] == 4294967295U);
}
TranspositionTable::~TranspositionTable(){
    delete[] this->head;
    delete[] this->next;
    delete[] this->key;
    delete[] this->val;
    delete[] this->flag;
    delete[] this->depth;
}
    //
void TranspositionTable::insert(ULL k, double v, short int f, int d){
    cerr << "[*] insert start \n";
    Size_ h = static_cast<Size_>(k & ((1ULL << 28) - 1ULL)); // ���᭱ 28 bit ���ন unsigned int
    assert(((k ^ static_cast<ULL>(h)) & ((1ULL << 28)-1ULL)) == 0ULL); // �̫ᨺ�@�� xor �����ӭn�O 0 
    next[this->size] = head[h]; // ���� next ����쥻 head ���쪺�a��
    head[h] = this->size; // �b�� head ���� �o�ӷs�� element �� index �W
    key[this->size] = k; // ���w key, value
    val[this->size] = v;
    flag[this->size] = f;
    depth[this->size] = d;
    
    this->size++;
    assert(this->size <= TranspositionTable::capacity);
}
// �� ULL �̫᪺ 28 �� bit
Record TranspositionTable::getVal(ULL k){
    Size_ h = static_cast<Size_>(k & ((1ULL << 28) - 1ULL));
    assert(((k ^ static_cast<ULL>(h)) & ((1ULL << 28)-1ULL)) == 0ULL);
    // �p�G i �O 32 bit �� 1, negation ���N�|�O 0
    for(unsigned int i = this->head[h]; ~i; i = this->next[i]){
        // �p�G key �Ȧ����
        if(key[i] == k){
            // �^�ǥL�̪� address
            cerr << "[*] Hit: " << this->val[i] << " " << this->flag[i] << " " << this->depth[i] << "\n";
            return Record(&(this->val[i]), &(this->flag[i]), &(this->depth[i]));
        }
    }
    return Record(nullptr, nullptr, nullptr);
}