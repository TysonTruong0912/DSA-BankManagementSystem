#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <mutex>
#include <string>
#include <vector>
#include "Account.h"

// Node cua danh sach lien ket don trong moi bucket (Separate Chaining)
struct Node
{
    Account data;
    Node* next;

    Node(const Account& acc) : data(acc), next(nullptr) {}
};

// Hash Table tu cai dat:
//   - Mang con tro Node** table, moi phan tu la dau mot linked list (bucket)
//   - Xu ly collision bang Separate Chaining
//   - Tu dong Rehash khi Load Factor > maxLoadFactor
//   - Thread-safe: moi thao tac public duoc bao ve bang std::mutex (C++11)
//     Phu hop multi-user / multi-thread doc-ghi dong thoi o muc bo nho dem.
class HashTable
{
private:
    Node** table;           // mang cac bucket
    int capacity;           // so bucket hien tai (so nguyen to)
    int size;               // so phan tu dang luu
    double maxLoadFactor;   // nguong rehash (0.75)
    int initialCapacity;    // capacity ban dau (dung khi clear)
    bool logRehash;         // true: in thong bao moi khi rehash
    mutable std::mutex mtx; // bao ve toan bo cau truc (mutable de lock trong ham const)

    static bool isPrime(int n);
    static int nextPrime(int n);        // so nguyen to nho nhat > n
    int hashWithCapacity(const std::string& key, int cap) const;

    // Phien ban khong lock (goi khi da giu mtx)
    bool insertUnlocked(const Account& account);
    Account* searchUnlocked(const std::string& accountNumber) const;
    void rehashUnlocked();

public:
    HashTable(int initialCapacity = 5);
    ~HashTable();

    // Khong cho phep sao chep HashTable (tranh hai doi tuong dung chung bucket)
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    // ---- Cac thao tac chinh (thread-safe) ----
    int hashFunction(const std::string& key) const;     // tra ve chi so bucket
    bool insert(const Account& account);                // false neu trung so tai khoan
    Account* search(const std::string& accountNumber) const;
    bool remove(const std::string& accountNumber);
    void rehash();
    void clear();                                       // xoa het du lieu

    // ---- Thong tin / thong ke ----
    double getLoadFactor() const;
    int getSize() const;
    int getCapacity() const;
    double getMaxLoadFactor() const;
    int getBucketLength(int index) const;
    int getChainPosition(const std::string& key) const; // vi tri trong chuoi (1..), 0 neu khong co
    int countEmptyBuckets() const;
    int countCollisions() const;        // so node nam "sau node dau" trong cac bucket
    int getLongestChain() const;
    void setLogRehash(bool value);

    // Tra ve con tro toi moi Account, da sap xep theo so tai khoan
    // Luu y: con tro chi an toan khi khong co thread khac dang remove/rehash/clear.
    // BankSystem goi trong pham vi da dong bo o tang tren.
    std::vector<Account*> getAllAccounts() const;

    // ---- Hien thi ----
    void display(bool onlyNonEmpty = false) const;  // tung bucket
    void displayStatistics() const;                 // thong ke
    void displayCollisions() const;                 // cac bucket co > 1 node
};

#endif
