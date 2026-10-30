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

// === BankSystem_Auth.cpp ===

bool BankSystem::dangNhapKhach()
{
    inTieuDe("ĐĂNG NHẬP KHÁCH HÀNG");
    cout << "  Dang nhap bang so tai khoan + PIN.\n\n";

    string soTK = docDong("Nhap so tai khoan (0 de huy): ");
    if (soTK == "0" || soTK.empty())
        return false;

    Account* acc = accounts.search(soTK);
    if (acc == nullptr)
    {
        baoLoi("Tai khoan khong ton tai.");
        return false;
    }
    if (!acc->getTrangThai())
    {
        baoLoi("Tai khoan da dong, khong the dang nhap.");
        return false;
    }
    if (!xacThucPin(acc))
        return false;

    currentUser = acc;
    vaiTro = VaiTro::KHACH;
    if (db.isOpen())
        db.audit("DANG_NHAP_KHACH", soTK, acc->getHoTen());
    UI::success("Xin chao, " + acc->getHoTen() + "!");
    cout << "  So TK: " << acc->getSoTaiKhoan()
         << " | So du: " << formatMoney(acc->getSoDu()) << " VND\n";
    vector<string> tb = layThongBaoNhanTien(acc, 5);
    if (!tb.empty())
        UI::notify("THONG BAO NHAN TIEN (gan day)", tb);
    else
        UI::info("Chua co thong bao nhan tien moi.");
    int pend = 0;
    for (size_t i = 0; i < dsYeuCau.size(); ++i)
        if (dsYeuCau[i].soTkTra == acc->getSoTaiKhoan() && dsYeuCau[i].trangThai == "CHO")
            ++pend;
    if (pend > 0)
        UI::warn("Co " + str(pend) + " yeu cau nhan tien dang cho ban xu ly.");
    return true;
}

bool BankSystem::dangNhapAdmin()
{
    inTieuDe("ĐĂNG NHẬP QUẢN TRỊ (ADMIN)");
    cout << "  Quyen: quan ly he thong. Khong duoc rut/chuyen tien.\n\n";

    string user = docDong("Ten dang nhap (0 de huy): ");
    if (user == "0" || user.empty())
        return false;
    string pass = docDong("Mat khau: ");

    if (user != ADMIN_USER || pass != ADMIN_PASS)
    {
        baoLoi("Sai ten dang nhap hoac mat khau.");
        if (db.isOpen())
            db.audit("DANG_NHAP_ADMIN_THAT_BAI", "", user);
        return false;
    }

    currentUser = nullptr;
    vaiTro = VaiTro::ADMIN;
    if (db.isOpen())
        db.audit("DANG_NHAP_ADMIN", "", user);
    cout << "\n[OK] Dang nhap Admin thanh cong.\n";
    return true;
}

void BankSystem::dangXuat()
{
    if (vaiTro == VaiTro::KHACH && currentUser != nullptr && db.isOpen())
        db.audit("DANG_XUAT", currentUser->getSoTaiKhoan(), "");
    else if (vaiTro == VaiTro::ADMIN && db.isOpen())
        db.audit("DANG_XUAT", "", "admin");

    currentUser = nullptr;
    vaiTro = VaiTro::KHONG;
    cout << "Da dang xuat.\n";
}

// ============================================================
// MAN HINH CHAO + MENU THEO VAI TRO
// ============================================================

void BankSystem::hienManHinhChao() const
{
    UI::clearScreen();
    UI::drawHeader();
    cout << "\n";

    vector<Account*> all = accounts.getAllAccounts();
    if (!all.empty())
    {
        UI::info("Danh sach tai khoan (toi da 5):");
        int show = (int)all.size() < 5 ? (int)all.size() : 5;
        for (int i = 0; i < show; ++i)
        {
            cout << "      " << all[i]->getSoTaiKhoan()
                 << "  -  " << all[i]->getHoTen()
                 << "  |  " << formatMoney(all[i]->getSoDu()) << " VND\n";
        }
        if ((int)all.size() > show)
            cout << "      ... va " << ((int)all.size() - show) << " TK khac\n";
    }
    else
        UI::info("Chua co tai khoan. Dang nhap Admin de mo TK moi.");

    UI::info("Ban demo hoc tap — thong tin dang nhap kiem thu: xem README.");
    cout << "\n";
}


