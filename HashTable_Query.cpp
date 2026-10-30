#include "HashTable.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;

// ============================================================
// Ham ho tro
// ============================================================


static bool accountLess(const Account* a, const Account* b)
{
    const string x = a->getSoTaiKhoan();
    const string y = b->getSoTaiKhoan();
    if (x.size() != y.size()) return x.size() < y.size();
    return x < y;
}
double HashTable::getLoadFactor() const
{
    lock_guard<mutex> lock(mtx);
    return (double)size / capacity;
}

int HashTable::getSize() const
{
    lock_guard<mutex> lock(mtx);
    return size;
}

int HashTable::getCapacity() const
{
    lock_guard<mutex> lock(mtx);
    return capacity;
}

double HashTable::getMaxLoadFactor() const { return maxLoadFactor; }

void HashTable::setLogRehash(bool value)
{
    lock_guard<mutex> lock(mtx);
    logRehash = value;
}

int HashTable::getBucketLength(int index) const
{
    lock_guard<mutex> lock(mtx);
    if (index < 0 || index >= capacity) return 0;
    int len = 0;
    for (Node* cur = table[index]; cur != nullptr; cur = cur->next)
        ++len;
    return len;
}

int HashTable::getChainPosition(const string& key) const
{
    lock_guard<mutex> lock(mtx);
    int index = hashWithCapacity(key, capacity);
    int pos = 1;
    for (Node* cur = table[index]; cur != nullptr; cur = cur->next, ++pos)
        if (cur->data.getSoTaiKhoan() == key)
            return pos;
    return 0;
}

int HashTable::countEmptyBuckets() const
{
    lock_guard<mutex> lock(mtx);
    int count = 0;
    for (int i = 0; i < capacity; ++i)
        if (table[i] == nullptr) ++count;
    return count;
}

int HashTable::countCollisions() const
{
    lock_guard<mutex> lock(mtx);
    int total = 0;
    for (int i = 0; i < capacity; ++i)
    {
        int len = 0;
        for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            ++len;
        if (len > 1) total += len - 1;
    }
    return total;
}

int HashTable::getLongestChain() const
{
    lock_guard<mutex> lock(mtx);
    int longest = 0;
    for (int i = 0; i < capacity; ++i)
    {
        int len = 0;
        for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            ++len;
        if (len > longest) longest = len;
    }
    return longest;
}

vector<Account*> HashTable::getAllAccounts() const
{
    lock_guard<mutex> lock(mtx);
    vector<Account*> result;
    for (int i = 0; i < capacity; ++i)
        for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            result.push_back(&(cur->data));
    sort(result.begin(), result.end(), accountLess);
    return result;
}

// ============================================================
// Hien thi

