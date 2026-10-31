#include "HashTable.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;

// ============================================================
// Ham ho tro
// ============================================================

bool HashTable::isPrime(int n)
{
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0) return false;
    return true;
}

int HashTable::nextPrime(int n)
{
    int x = n + 1;
    while (!isPrime(x)) ++x;
    return x;
}

static bool accountLess(const Account* a, const Account* b)
{
    const string x = a->getSoTaiKhoan();
    const string y = b->getSoTaiKhoan();
    if (x.size() != y.size()) return x.size() < y.size();
    return x < y;
}

// ============================================================
// Constructor / Destructor
// ============================================================

HashTable::HashTable(int initCap)
    : table(nullptr), capacity(initCap), size(0), maxLoadFactor(0.75),
      initialCapacity(initCap), logRehash(true)
{
    table = new Node*[capacity];
    for (int i = 0; i < capacity; ++i)
        table[i] = nullptr;
}

HashTable::~HashTable()
{
    lock_guard<mutex> lock(mtx);
    for (int i = 0; i < capacity; ++i)
    {
        Node* cur = table[i];
        while (cur != nullptr)
        {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
    }
    delete[] table;
}

// ============================================================
// HASH FUNCTION
// ============================================================

int HashTable::hashWithCapacity(const string& key, int cap) const
{
    unsigned long long hash = 0;
    for (size_t i = 0; i < key.size(); ++i)
        hash = (hash * 31 + (unsigned char)key[i]) % cap;
    return (int)hash;
}

int HashTable::hashFunction(const string& key) const
{
    lock_guard<mutex> lock(mtx);
    return hashWithCapacity(key, capacity);
}

// ============================================================
// INSERT (thread-safe) - trung binh O(1)
// ============================================================
bool HashTable::insertUnlocked(const Account& account)
{
    int index = hashWithCapacity(account.getSoTaiKhoan(), capacity);

    for (Node* cur = table[index]; cur != nullptr; cur = cur->next)
    {
        if (cur->data.getSoTaiKhoan() == account.getSoTaiKhoan())
            return false;
    }

    Node* newNode = new Node(account);
    newNode->next = table[index];
    table[index] = newNode;
    ++size;

    if ((double)size / capacity > maxLoadFactor)
        rehashUnlocked();

    return true;
}

bool HashTable::insert(const Account& account)
{
    lock_guard<mutex> lock(mtx);
    return insertUnlocked(account);
}

// ============================================================
// SEARCH (thread-safe)
// ============================================================
Account* HashTable::searchUnlocked(const string& accountNumber) const
{
    int index = hashWithCapacity(accountNumber, capacity);
    for (Node* cur = table[index]; cur != nullptr; cur = cur->next)
    {
        if (cur->data.getSoTaiKhoan() == accountNumber)
            return &(cur->data);
    }
    return nullptr;
}

Account* HashTable::search(const string& accountNumber) const
{
    lock_guard<mutex> lock(mtx);
    return searchUnlocked(accountNumber);
}

// ============================================================
// REMOVE (thread-safe)
// ============================================================
bool HashTable::remove(const string& accountNumber)
{
    lock_guard<mutex> lock(mtx);
    int index = hashWithCapacity(accountNumber, capacity);

    Node* prev = nullptr;
    Node* cur = table[index];

    while (cur != nullptr)
    {
        if (cur->data.getSoTaiKhoan() == accountNumber)
        {
            if (prev == nullptr)
                table[index] = cur->next;
            else
                prev->next = cur->next;
            delete cur;
            --size;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

// ============================================================
// REHASH (thread-safe)
// ============================================================
void HashTable::rehashUnlocked()
{
    int oldCapacity = capacity;
    int newCapacity = nextPrime(oldCapacity * 2);

    Node** newTable = new Node*[newCapacity];
    for (int i = 0; i < newCapacity; ++i)
        newTable[i] = nullptr;

    for (int i = 0; i < oldCapacity; ++i)
    {
        Node* cur = table[i];
        while (cur != nullptr)
        {
            Node* next = cur->next;
            int newIndex = hashWithCapacity(cur->data.getSoTaiKhoan(), newCapacity);
            cur->next = newTable[newIndex];
            newTable[newIndex] = cur;
            cur = next;
        }
    }

    delete[] table;
    table = newTable;
    capacity = newCapacity;

    if (logRehash)
    {
        cout << "[REHASH] capacity " << oldCapacity << " -> " << newCapacity
             << " | size = " << size
             << " | load factor = " << fixed << setprecision(4)
             << ((double)size / capacity) << "\n";
        cout.unsetf(ios::floatfield);
    }
}

void HashTable::rehash()
{
    lock_guard<mutex> lock(mtx);
    rehashUnlocked();
}

void HashTable::clear()
{
    lock_guard<mutex> lock(mtx);
    for (int i = 0; i < capacity; ++i)
    {
        Node* cur = table[i];
        while (cur != nullptr)
        {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
    }
    delete[] table;

    capacity = initialCapacity;
    size = 0;
    table = new Node*[capacity];
    for (int i = 0; i < capacity; ++i)
        table[i] = nullptr;
}

// ============================================================
// Thong tin / thong ke (thread-safe)
// ============================================================

