#include "BankSystem.h"
#include "BankHelpers.h"
#include "FileManager.h"
#include "Security.h"
#include "UI.h"
#include "Utils.h"
#include <algorithm>
#include <mutex>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
using namespace std;
using namespace BankHelpers;

// === BankSystem_MenuDisplay.cpp ===

void BankSystem::hienMenuKhach() const
{
    UI::clearScreen();
    UI::drawHeader();
    if (currentUser != nullptr)
    {
        cout << "\n";
        vector<pair<string, string> > cards;
        cards.push_back(make_pair("Chu tai khoan", currentUser->getHoTen()));
        cards.push_back(make_pair("So tai khoan", currentUser->getSoTaiKhoan()));
        cards.push_back(make_pair("So du", formatMoney(currentUser->getSoDu()) + " VND"));
        string loai = (currentUser->getLoaiTaiKhoan() == TIET_KIEM) ? "Tiet kiem" : "Thanh toan";
        cards.push_back(make_pair("Loai TK", loai));
        UI::drawCards(cards);
        cout << "\n";
        vector<string> tb = layThongBaoNhanTien(currentUser, 3);
        if (!tb.empty())
            UI::notify("TIEN DEN TAI KHOAN CUA BAN", tb);
    }
}

void BankSystem::hienMenuAdmin() const
{
    UI::clearScreen();
    UI::drawHeader();
    vector<Account*> all = accounts.getAllAccounts();
    int dangHoatDong = 0, daDong = 0;
    double tongSoDu = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i]->getTrangThai()) ++dangHoatDong;
        else ++daDong;
        tongSoDu += all[i]->getSoDu();
    }
    UI::drawStatusBar((int)all.size(), dangHoatDong, daDong, tongSoDu,
                      db.isOpen(), getCurrentDateTime());
    UI::info("Quyen: QUAN TRI  |  Khong rut / chuyen tien");
}


// ============================================================
// MOMO-LIKE: chuyen SĐT, yeu cau tien, hoa don, nap DT, hu TK, help
// ============================================================


