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

// === BankSystem_SaoKe.cpp ===

void BankSystem::xemSaoKe()
{
    inTieuDe("XEM SAO KÊ");

    Account* acc = (vaiTro == VaiTro::KHACH) ? currentUser : layTaiKhoan("Nhap so tai khoan: ");
    if (acc == nullptr) return;

    cout << "So tai khoan : " << acc->getSoTaiKhoan() << " | Chu tai khoan: " << acc->getHoTen()
         << " | So du: " << formatMoney(acc->getSoDu()) << " VND\n\n";
    cout << "1. Tat ca giao dich\n";
    cout << "2. N giao dich gan nhat\n";
    cout << "3. Theo khoang ngay\n";
    cout << "4. Theo loai giao dich\n";
    cout << "0. Huy\n";
    string choice = docDong("Nhập lựa chọn: ");

    vector<const Transaction*> tatCa = layGiaoDich(*acc);
    vector<const Transaction*> ds;

    if (choice == "1")
    {
        ds = tatCa;
    }
    else if (choice == "2")
    {
        string s = docDong("Nhap N (1-1000): ");
        if (!isAllDigits(s) || s.size() > 4 || atoi(s.c_str()) < 1 || atoi(s.c_str()) > 1000)
        {
            baoLoi("N phai la so nguyen tu 1 den 1000.");
            return;
        }
        size_t n = (size_t)atoi(s.c_str());
        size_t batDau = tatCa.size() > n ? tatCa.size() - n : 0;
        ds.assign(tatCa.begin() + batDau, tatCa.end());
    }
    else if (choice == "3")
    {
        int tu, den;
        if (!parseDateInput(docDong("Tu ngay (dd/mm/yyyy): "), tu) ||
            !parseDateInput(docDong("Den ngay (dd/mm/yyyy): "), den))
        {
            baoLoi("Ngay khong hop le. Dinh dang dung: dd/mm/yyyy (vi du 07/10/2026).");
            return;
        }
        if (tu > den)
        {
            baoLoi("Ngay bat dau phai truoc hoac bang ngay ket thuc.");
            return;
        }
        for (size_t i = 0; i < tatCa.size(); ++i)
        {
            int k = dateKey(tatCa[i]->getThoiGian());
            if (k >= tu && k <= den) ds.push_back(tatCa[i]);
        }
    }
    else if (choice == "4")
    {
        cout << "1. Nap tien  2. Rut tien  3. Chuyen khoan di  4. Nhan chuyen khoan  5. Tra lai\n";
        string l = docDong("Chon loai: ");
        TransactionType loai;
        if (l == "1") loai = NAP_TIEN;
        else if (l == "2") loai = RUT_TIEN;
        else if (l == "3") loai = CHUYEN_KHOAN;
        else if (l == "4") loai = NHAN_CHUYEN_KHOAN;
        else if (l == "5") loai = TRA_LAI;
        else
        {
            baoLoi("Loai giao dich khong hop le.");
            return;
        }
        for (size_t i = 0; i < tatCa.size(); ++i)
            if (tatCa[i]->getLoai() == loai) ds.push_back(tatCa[i]);
    }
    else
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }

    cout << "\nSAO KE TAI KHOAN " << acc->getSoTaiKhoan() << "\n";
    inBangGiaoDich(ds);

    if (!ds.empty() && xacNhan("Xuat sao ke ra file CSV?"))
    {
        string file = "saoke_" + acc->getSoTaiKhoan() + "_" + getTimestampForFile() + ".csv";
        if (xuatCsvSaoKe(file, ds))
            cout << "[OK] Da xuat " << ds.size() << " giao dich ra file " << file << "\n";
        else
            baoLoi("Khong the ghi file " + file + ".");
    }
}

// ============================================================
// 11. TRA CUU GIAO DICH THEO MA
// ============================================================
void BankSystem::traCuuGiaoDich()
{
    inTieuDe("TRA CUU GIAO DICH THEO MA");

    string s = docDong("Nhap ma giao dich (vi du GD000012 hoac 12): ");
    if (s.empty())
    {
        baoLoi("Ma giao dich khong duoc de trong.");
        return;
    }
    string ma;
    if (isAllDigits(s) && s.size() <= 9)
        ma = maGiaoDich(atoi(s.c_str()));
    else
    {
        ma = s;
        for (size_t i = 0; i < ma.size(); ++i) ma[i] = (char)toupper((unsigned char)ma[i]);
    }

    vector<Account*> all = accounts.getAllAccounts();
    for (size_t i = 0; i < all.size(); ++i)
    {
        for (TransactionNode* cur = all[i]->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            if (cur->data.getMaGiaoDich() != ma) continue;

            const Transaction& t = cur->data;
            cout << "\n[Giao dich tim thay]\n";
            inChiTietGiaoDich(all[i]->getSoTaiKhoan(), all[i]->getHoTen(), t);

            // Chuyen khoan co 2 ban ghi (ben gui / ben nhan) -> tim ban ghi doi ung
            if ((t.getLoai() == CHUYEN_KHOAN || t.getLoai() == NHAN_CHUYEN_KHOAN) &&
                !t.getTaiKhoanLienQuan().empty())
            {
                Account* other = accounts.search(t.getTaiKhoanLienQuan());
                if (other != nullptr)
                {
                    TransactionType doiUng = (t.getLoai() == CHUYEN_KHOAN) ? NHAN_CHUYEN_KHOAN : CHUYEN_KHOAN;
                    for (TransactionNode* o = other->getTransactionHead(); o != nullptr; o = o->next)
                    {
                        if (o->data.getLoai() == doiUng && o->data.getThoiGian() == t.getThoiGian() &&
                            o->data.getTaiKhoanLienQuan() == all[i]->getSoTaiKhoan() &&
                            fabs(o->data.getSoTien() - t.getSoTien()) < 0.005)
                        {
                            cout << "\n[Ban ghi doi ung]\n";
                            inChiTietGiaoDich(other->getSoTaiKhoan(), other->getHoTen(), o->data);
                            break;
                        }
                    }
                }
            }
            return;
        }
    }
    baoLoi("Khong tim thay giao dich " + ma + ".");
}

// ============================================================


