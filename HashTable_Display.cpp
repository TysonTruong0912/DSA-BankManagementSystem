#include "HashTable.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;

// ============================================================
// Ham ho tro
// ============================================================

void HashTable::display(bool onlyNonEmpty) const
{
    lock_guard<mutex> lock(mtx);
    cout << "====================================================\n";
    cout << "CAU TRUC HASH TABLE (Separate Chaining)\n";
    cout << "====================================================\n";
    for (int i = 0; i < capacity; ++i)
    {
        if (onlyNonEmpty && table[i] == nullptr) continue;
        cout << "Bucket " << i << ": ";
        for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            cout << cur->data.getSoTaiKhoan() << " -> ";
        cout << "NULL\n";
    }
}

void HashTable::displayStatistics() const
{
    lock_guard<mutex> lock(mtx);
    int empty = 0;
    int collisions = 0;
    int longest = 0;
    for (int i = 0; i < capacity; ++i)
    {
        int len = 0;
        for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            ++len;
        if (len == 0) ++empty;
        if (len > 1) collisions += len - 1;
        if (len > longest) longest = len;
    }

    cout << "====================================================\n";
    cout << "THONG KE HASH TABLE\n";
    cout << "====================================================\n";
    cout << "Capacity             : " << capacity << "\n";
    cout << "So luong (size)      : " << size << "\n";
    cout << "Load Factor          : " << fixed << setprecision(4) << ((double)size / capacity)
         << " (= size / capacity)\n";
    cout << "Nguong Rehash        : " << setprecision(2) << maxLoadFactor << "\n";
    cout.unsetf(ios::floatfield);
    cout << "So bucket            : " << capacity << "\n";
    cout << "So bucket rong       : " << empty << "\n";
    cout << "So bucket co du lieu : " << (capacity - empty) << "\n";
    cout << "So collision         : " << collisions << "\n";
    cout << "Chuoi dai nhat       : " << longest << " node\n";
    cout << "Ham bam              : hash = hash * 31 + c ; index = hash % capacity\n";
    cout << "Dong bo              : std::mutex (thread-safe)\n";
    cout << "====================================================\n";
}

void HashTable::displayCollisions() const
{
    lock_guard<mutex> lock(mtx);
    cout << "====================================================\n";
    cout << "KIEM TRA COLLISION\n";
    cout << "====================================================\n";

    int collisionBuckets = 0;
    for (int i = 0; i < capacity; ++i)
    {
        int len = 0;
        for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            ++len;
        if (len > 1)
        {
            ++collisionBuckets;
            cout << "Bucket " << i << ":\n";
            for (Node* cur = table[i]; cur != nullptr; cur = cur->next)
            {
                cout << cur->data.getSoTaiKhoan();
                if (cur->next != nullptr) cout << " -> ";
            }
            cout << "\n\n";
        }
    }

    if (collisionBuckets == 0)
        cout << "Khong co collision nao (moi bucket co toi da 1 node).\n";

    cout << "----------------------------------------------------\n";
    cout << "So bucket co collision : " << collisionBuckets << "\n";
}

