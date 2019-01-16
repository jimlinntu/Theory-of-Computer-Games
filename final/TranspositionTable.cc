#include"TranspositionTable.hh"
#include<bitset>
#include<assert.h>
// 檢查完畢
TranspositionTable::TranspositionTable(){
    this->size = 0;
    this->head = new Size_[TranspositionTable::capacity];
    this->next = new Size_[TranspositionTable::capacity];
    this->key = new ULL[TranspositionTable::capacity];
    this->val = new double[TranspositionTable::capacity];
    this->flag = new short int[TranspositionTable::capacity];
    this->depth = new int[TranspositionTable::capacity];
    
    // 放入 32 bit 全為 1 的數字(之後取 negation 比較方便)
    fill(this->head, this->head+capacity, (0U-1U));
    cerr << "head[0]" << bitset<32>(this->head[0]) << "\n";
    assert(this->head[0] == 4294967295U && this->head[0] == 4294967295U);
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
    Size_ h = static_cast<Size_>(k & ((1ULL << 28) - 1ULL)); // 取後面 28 bit 並轉成 unsigned int
    cerr << "k:"<< bitset<64>(k) << "\n"; 
    cerr << "h:"<< bitset<64>(h) << "\n";
    assert(((k ^ static_cast<ULL>(h)) & ((1ULL << 28)-1ULL)) == 0ULL); // 最後那一串 xor 完應該要是 0 
    next[this->size] = head[h]; // 先把 next 指到原本 head 指到的地方
    head[h] = this->size; // 在把 head 指到 這個新的 element 的 index 上
    key[this->size] = k; // 指定 key, value
    val[this->size] = v;
    flag[this->size] = f;
    depth[this->size] = d;
    
    this->size++;
    assert(this->size <= TranspositionTable::capacity);
}
// 取 ULL 最後的 28 個 bit
Record TranspositionTable::getVal(ULL k){
    Size_ h = static_cast<Size_>(k & ((1ULL << 28) - 1ULL));
    cerr << "k:"<< bitset<64>(k) << "\n"; 
    cerr << "h:"<< bitset<64>(h) << "\n";
    assert(((k ^ static_cast<ULL>(h)) & ((1ULL << 28)-1ULL)) == 0ULL);
    // 如果 i 是 32 bit 全 1, negation 完就會是 0
    for(unsigned int i = this->head[h]; ~i; i = this->next[i]){
        cerr << "[*]      i: " << bitset<64>(i) << "\n";
        cerr << "[*] key[i]:" << bitset<64>(key[i]) << "\n";
        // 如果 key 值有對到
        if(key[i] == k){
            // 回傳他們的 address
            cerr << "[*] Hit: " << this->val[i] << " " << this->flag[i] << " " << this->depth[i] << "\n";
            return Record(&(this->val[i]), &(this->flag[i]), &(this->depth[i]));
        }
    }
    return Record(nullptr, nullptr, nullptr);
}