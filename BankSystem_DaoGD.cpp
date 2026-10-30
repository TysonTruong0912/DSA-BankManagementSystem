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

// === BankSystem_DaoGD.cpp ===

void BankSystem::daoGiaoDich()
{
    inTieuDe("ĐẢO / HOÀN GIAO DỊCH");

    string ma = docDong("Nhap ma giao dich can dao (vi du GD000012, 0 de huy): ");
    if (ma == "0" || ma.empty())
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }

    // Tim giao dich trong toan bo he thong
    vector<Account*> all = accounts.getAllAccounts();
    Account* accGoc = nullptr;
    const Transaction* txGoc = nullptr;
    for (size_t i = 0; i < all.size() && txGoc == nullptr; ++i)
    {
        for (TransactionNode* cur = all[i]->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            if (cur->data.getMaGiaoDich() == ma)
            {
                accGoc = all[i];
                txGoc = &cur->data;
                break;
            }
        }
    }
    if (txGoc == nullptr)
    {
        baoLoi("Khong tim thay giao dich " + ma + ".");
        return;
    }

    // Kiem tra da bi dao chua
    for (size_t i = 0; i < all.size(); ++i)
    {
        for (TransactionNode* cur = all[i]->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            if (cur->data.getLoai() == DAO_GIAO_DICH && cur->data.getMaGiaoDichGoc() == ma)
            {
                baoLoi("Giao dich " + ma + " da bi dao truoc do (ma dao: " + cur->data.getMaGiaoDich() + ").");
                return;
            }
        }
    }

    TransactionType loai = txGoc->getLoai();
    if (loai == TRA_LAI || loai == DAO_GIAO_DICH)
    {
        baoLoi("Khong the dao loai giao dich " + Transaction::typeToString(loai) + ".");
        return;
    }
    if (!accGoc->getTrangThai())
    {
        baoLoi("Tai khoan " + accGoc->getSoTaiKhoan() + " da dong, khong the dao giao dich.");
        return;
    }

    cout << "\nGiao dich goc:\n";
    inChiTietGiaoDich(accGoc->getSoTaiKhoan(), accGoc->getHoTen(), *txGoc);

    if (!xacNhan("Xac nhan DAO giao dich " + ma))
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }
    if (!xacThucPin(accGoc)) return;

    vector<Buoc> cacBuoc;
    double soTien = txGoc->getSoTien();
    string lienQuan = txGoc->getTaiKhoanLienQuan();

    if (loai == NAP_TIEN)
    {
        // Dao nap = tru tien
        if (accGoc->getSoDu() < soTien)
        {
            baoLoi("So du khong du de dao nap tien.");
            return;
        }
        Buoc b = {accGoc, accGoc->getSoDu() - soTien, DAO_GIAO_DICH, soTien, "",
                  "Dao GD " + ma + " (NAP_TIEN)", ma};
        cacBuoc.push_back(b);
    }
    else if (loai == RUT_TIEN || loai == PHI_GIAO_DICH)
    {
        // Dao rut/phi = cong tien lai
        if (accGoc->getSoDu() + soTien > MAX_BALANCE)
        {
            baoLoi("So du se vuot gioi han khi dao.");
            return;
        }
        Buoc b = {accGoc, accGoc->getSoDu() + soTien, DAO_GIAO_DICH, soTien, "",
                  "Dao GD " + ma + " (" + Transaction::typeToString(loai) + ")", ma};
        cacBuoc.push_back(b);
    }
    else if (loai == CHUYEN_KHOAN)
    {
        // Dao chuyen: cong lai nguon, tru dich
        Account* accDich = accounts.search(lienQuan);
        if (accDich == nullptr || !accDich->getTrangThai())
        {
            baoLoi("Tai khoan dich " + lienQuan + " khong ton tai hoac da dong.");
            return;
        }
        if (accDich->getSoDu() < soTien)
        {
            baoLoi("Tai khoan dich khong du so du de dao chuyen khoan.");
            return;
        }
        if (accGoc->getSoDu() + soTien > MAX_BALANCE)
        {
            baoLoi("So du tai khoan nguon se vuot gioi han.");
            return;
        }
        Buoc bA = {accGoc, accGoc->getSoDu() + soTien, DAO_GIAO_DICH, soTien, lienQuan,
                   "Dao GD " + ma + " (CHUYEN_KHOAN - hoan ve)", ma};
        Buoc bB = {accDich, accDich->getSoDu() - soTien, DAO_GIAO_DICH, soTien, accGoc->getSoTaiKhoan(),
                   "Dao GD " + ma + " (NHAN_CHUYEN_KHOAN - thu hoi)", ma};
        cacBuoc.push_back(bA);
        cacBuoc.push_back(bB);
    }
    else if (loai == NHAN_CHUYEN_KHOAN)
    {
        baoLoi("Hay dao tu ma GD ben CHUYEN_KHOAN (cung se dao ca 2 ben).");
        return;
    }
    else
    {
        baoLoi("Loai giao dich khong ho tro dao.");
        return;
    }

    if (!thucThi(cacBuoc, "DAO_GD", "dao " + ma + " | " + Transaction::typeToString(loai)))
        return;

    cout << "[OK] Da dao giao dich " << ma << ".\n";
    for (size_t i = 0; i < cacBuoc.size(); ++i)
        cout << "     So du " << cacBuoc[i].acc->getSoTaiKhoan() << ": "
             << formatMoney(cacBuoc[i].acc->getSoDu()) << " VND\n";
}

// ============================================================


