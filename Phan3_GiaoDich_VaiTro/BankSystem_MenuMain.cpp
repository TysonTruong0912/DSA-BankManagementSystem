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

// === BankSystem_MenuMain.cpp ===

void BankSystem::menu()
{
    while (true)
    {
        hienManHinhChao();
        vector<string> opts;
        opts.push_back("1. Dang nhap Khach hang");
        opts.push_back("2. Dang nhap Quan tri (Admin)");
        opts.push_back("0. Thoat chuong trinh");
        int sel = UI::selectMenu("NEXUS BANK - DANG NHAP", opts, 0);
        if (sel == 0)
        {
            if (dangNhapKhach())
                menuKhach();
            else
                tamDung();
        }
        else if (sel == 1)
        {
            if (dangNhapAdmin())
                menuAdmin();
            else
                tamDung();
        }
        else
        {
            if (db.isOpen())
                cout << "Du lieu da luu tu dong trong " << db.path() << ".\n";
            else if (xacNhan("Ban co muon luu du lieu truoc khi thoat?"))
                luuDuLieu();
            cout << "Tam biet!\n";
            return;
        }
    }
}


