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

// === BankSystem_DataAdmin.cpp ===

void BankSystem::xemNhatKy()
{
    inTieuDe("NHẬT KÝ HỆ THỐNG (AUDIT LOG)");
    if (!db.isOpen())
    {
        baoLoi("Nhat ky chi co khi he thong chay voi co so du lieu SQLite.");
        return;
    }
    string s = docDong("Xem bao nhieu dong gan nhat (1-500, Enter = 20): ");
    int n = 20;
    if (!s.empty())
    {
        if (!isAllDigits(s) || s.size() > 3 || atoi(s.c_str()) < 1 || atoi(s.c_str()) > 500)
        {
            baoLoi("Vui long nhap so nguyen tu 1 den 500.");
            return;
        }
        n = atoi(s.c_str());
    }
    cout << "\n";
    if (!db.printAudit(n))
        baoLoi("Khong doc duoc nhat ky: " + db.lastError());
}

// ============================================================
// 20. SAO LUU CSDL
// ============================================================
void BankSystem::saoLuu()
{
    inTieuDe("SAO LƯU CƠ SỞ DỮ LIỆU");
    if (!db.isOpen())
    {
        baoLoi("Chua co CSDL SQLite. Hay dung 'Xuat file .txt' trong menu 21 de sao luu.");
        return;
    }
    string file = "backup_" + getTimestampForFile() + ".db";
    if (db.backupTo(file))
    {
        db.audit("SAO_LUU", "", file);
        cout << "[OK] Da sao luu CSDL ra file " << file << "\n";
        cout << "     Khoi phuc: dong chuong trinh, doi ten file nay thanh " << DB_FILE << " roi chay lai.\n";
    }
    else
        baoLoi("Sao luu that bai: " + db.lastError());
}

// ============================================================
// Xuat / nhap file .txt
// ============================================================
void BankSystem::luuDuLieu()
{
    inTieuDe("XUẤT FILE .TXT");
    if (FileManager::saveAll(accounts, lastTxId, ACCOUNT_FILE, TRANSACTION_FILE))
        cout << "[OK] Da luu " << accounts.getSize() << " tai khoan vao " << ACCOUNT_FILE
             << " va giao dich vao " << TRANSACTION_FILE << "\n";
    else
        baoLoi("Khong the ghi file. Kiem tra quyen ghi trong thu muc hien tai.");
}

void BankSystem::docDuLieu()
{
    inTieuDe("NHẬP FILE .TXT");
    string canhBao = db.isOpen() ? "Du lieu hien tai (ke ca trong CSDL) se bi THAY THE boi noi dung file. Tiep tuc?"
                                 : "Du lieu hien tai trong bo nho se bi thay the. Tiep tuc?";
    if (!xacNhan(canhBao))
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }

    accounts.setLogRehash(false);
    bool ok = FileManager::loadAll(accounts, lastTxId, ACCOUNT_FILE, TRANSACTION_FILE);
    if (ok && db.isOpen() && !dongBoCSDL())
    {
        // Ghi CSDL that bai -> doc lai CSDL de bo nho khop voi CSDL
        db.loadAll(accounts, lastTxId);
        accounts.setLogRehash(true);
        baoLoi("Khong ghi duoc CSDL, da khoi phuc du lieu tu CSDL.");
        return;
    }
    accounts.setLogRehash(true);

    if (ok)
        cout << "[OK] Da doc " << accounts.getSize() << " tai khoan. Hash Table duoc khoi phuc: capacity = "
             << accounts.getCapacity() << "\n";
    else
        baoLoi("Khong tim thay file " + ACCOUNT_FILE + ". Du lieu hien tai duoc giu nguyen.");
}

// ============================================================
// 21. QUAN LY DU LIEU (menu con)
// ============================================================
void BankSystem::quanLyDuLieu()
{
    while (true)
    {
        cout << "\n----------------------------------------------------\n";
        cout << "QUAN LY DU LIEU\n";
        cout << "----------------------------------------------------\n";
        cout << "1. Xuat file .txt (accounts.txt / transactions.txt)\n";
        cout << "2. Nhap file .txt (thay the du lieu hien tai)\n";
        cout << "3. Xuat danh sach tai khoan ra CSV\n";
        cout << "4. Khoi phuc du lieu mau\n";
        cout << "5. Don dep tai khoan test (Test User, so du 0)\n";
        cout << "6. Doi soat & kiem tra toan ven du lieu\n";
        cout << "0. Quay lai\n";
        string choice = docDong("Nhập lựa chọn: ");

        if (choice == "1")
            luuDuLieu();
        else if (choice == "2")
            docDuLieu();
        else if (choice == "3")
        {
            vector<Account*> all = accounts.getAllAccounts();
            string file = "taikhoan_" + getTimestampForFile() + ".csv";
            ofstream out(file.c_str());
            if (!out)
            {
                baoLoi("Khong the ghi file " + file + ".");
                continue;
            }
            out << fixed << setprecision(2);
            out << "so_tai_khoan,ho_ten,so_dien_thoai,email,so_du,trang_thai,ngay_mo,so_giao_dich\n";
            for (size_t i = 0; i < all.size(); ++i)
                out << csvEscape(all[i]->getSoTaiKhoan()) << "," << csvEscape(all[i]->getHoTen()) << ","
                    << csvEscape(all[i]->getSoDienThoai()) << "," << csvEscape(all[i]->getEmail()) << ","
                    << all[i]->getSoDu() << "," << (all[i]->getTrangThai() ? "hoat_dong" : "da_dong") << ","
                    << csvEscape(all[i]->getNgayMo()) << "," << all[i]->getTransactionCount() << "\n";
            cout << "[OK] Da xuat " << all.size() << " tai khoan ra file " << file << "\n";
        }
        else if (choice == "4")
        {
            inTieuDe("KHÔI PHỤC DỮ LIỆU MẪU");
            if (xacNhan("Toan bo du lieu hien tai se bi xoa. Tiep tuc?"))
            {
                accounts.setLogRehash(false);
                taoDuLieuMau();
                accounts.setLogRehash(true);
                dongBoCSDL();
                cout << "[OK] Da tao lai " << accounts.getSize() << " tai khoan mau (PIN: " << PIN_MAU << ").\n";
            }
        }
        else if (choice == "5")
            donDepTaiKhoanTest();
        else if (choice == "6")
            doiSoat();
        else if (choice == "0")
            return;
        else
            baoLoi("Lựa chọn không hợp lệ.");
    }
}

// Xoa cac tai khoan "Test User" do demo rehash phien ban cu de lai
void BankSystem::donDepTaiKhoanTest()
{
    inTieuDe("DỌN DẸP TÀI KHOẢN TEST");
    vector<Account*> all = accounts.getAllAccounts();
    vector<string> ds;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i]->getHoTen() == "Test User" && all[i]->getEmail() == "test@demo.com" &&
            all[i]->getSoDu() == 0 && all[i]->getTransactionCount() == 0)
            ds.push_back(all[i]->getSoTaiKhoan());

    if (ds.empty())
    {
        cout << "Khong co tai khoan test nao can don dep.\n";
        return;
    }
    cout << "Tim thay " << ds.size() << " tai khoan test (so du 0, khong co giao dich).\n";
    if (!xacNhan("Xoa vinh vien cac tai khoan nay"))
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }
    if (db.isOpen())
    {
        bool ok = db.begin();
        for (size_t i = 0; ok && i < ds.size(); ++i) ok = db.deleteAccount(ds[i]);
        if (ok) ok = db.audit("DON_DEP_TEST", "", str((long long)ds.size()) + " tai khoan");
        if (ok) ok = db.commit();
        if (!ok)
        {
            string e = db.lastError();
            db.rollback();
            baoLoi("Loi co so du lieu: " + e + ". Khong co tai khoan nao bi xoa.");
            return;
        }
    }
    for (size_t i = 0; i < ds.size(); ++i) accounts.remove(ds[i]);
    cout << "[OK] Da xoa " << ds.size() << " tai khoan test. Con lai " << accounts.getSize() << " tai khoan.\n";
}

// Doi soat: kiem tra so du khop lich su, chuoi giao dich lien tuc, bo nho khop CSDL
void BankSystem::doiSoat()
{
    inTieuDe("ĐỐI SOÁT & KIỂM TRA TOÀN VẸN");

    vector<Account*> all = accounts.getAllAccounts();
    int loi = 0;
    long long tongGd = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        const Account* a = all[i];
        double prev = 0;
        for (TransactionNode* cur = a->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            const Transaction& t = cur->data;
            ++tongGd;
            bool ra = laTienRa(t.getLoai());
            if (t.getLoai() == DAO_GIAO_DICH)
                ra = (t.getSoDuSau() < t.getSoDuTruoc());
            double ky = round2(t.getSoDuTruoc() + (ra ? -t.getSoTien() : t.getSoTien()));
            if (fabs(ky - t.getSoDuSau()) > 0.005)
            {
                cout << "[SAI] " << t.getMaGiaoDich() << " (TK " << a->getSoTaiKhoan()
                     << "): so du truoc/sau khong khop so tien giao dich.\n";
                ++loi;
            }
            if (fabs(t.getSoDuTruoc() - prev) > 0.005)
            {
                cout << "[SAI] " << t.getMaGiaoDich() << " (TK " << a->getSoTaiKhoan()
                     << "): so du truoc khong noi tiep giao dich lien truoc.\n";
                ++loi;
            }
            prev = t.getSoDuSau();
        }
        if (fabs(prev - a->getSoDu()) > 0.005)
        {
            cout << "[SAI] TK " << a->getSoTaiKhoan() << ": so du hien tai " << formatMoney(a->getSoDu())
                 << " khac so du theo lich su " << formatMoney(prev) << ".\n";
            ++loi;
        }
    }
    cout << "Da kiem tra " << all.size() << " tai khoan, " << tongGd << " giao dich: "
         << (loi == 0 ? "KHOP HOAN TOAN." : str(loi) + " loi (xem o tren).") << "\n";

    if (db.isOpen())
    {
        string kq;
        if (db.integrityCheck(kq))
            cout << "SQLite integrity_check: " << kq << "\n";
        long long nTk = db.count("accounts"), nGd = db.count("transactions");
        bool khop = (nTk == (long long)all.size() && nGd == tongGd);
        cout << "Bo nho: " << all.size() << " TK, " << tongGd << " GD | CSDL: " << nTk << " TK, " << nGd
             << " GD -> " << (khop ? "KHOP." : "LECH! Hay dung menu 21 -> 2 hoac khoi dong lai de dong bo.") << "\n";
    }
    else
        cout << "(Khong co CSDL de doi chieu - che do file .txt)\n";
}

// ============================================================
// 22. KIEM TRA HASH TABLE
// ============================================================
void BankSystem::kiemTraHashTable()
{
    while (true)
    {
        cout << "\n----------------------------------------------------\n";
        cout << "KIEM TRA HASH TABLE\n";
        cout << "----------------------------------------------------\n";
        cout << "1. Thong ke Hash Table\n";
        cout << "2. Hien thi tung bucket\n";
        cout << "3. Kiem tra collision\n";
        cout << "4. Demo Rehash (bang tam, KHONG anh huong du lieu that)\n";
        cout << "0. Quay lai menu chinh\n";
        string choice = docDong("Nhập lựa chọn: ");

        if (choice == "1")
            accounts.displayStatistics();
        else if (choice == "2")
        {
            bool chiCoDuLieu = xacNhan("Chi hien thi bucket co du lieu?");
            accounts.display(chiCoDuLieu);
        }
        else if (choice == "3")
            accounts.displayCollisions();
        else if (choice == "4")
            demoRehash();
        else if (choice == "0")
            return;
        else
            baoLoi("Lựa chọn không hợp lệ.");
    }
}

// Demo tren mot Hash Table tam (capacity 5) -> xem 5 -> 11 -> 23 -> 47 -> 97 ... ma khong
// them tai khoan rac vao du lieu that / CSDL.
void BankSystem::demoRehash()
{
    string s = docDong("Nhap so tai khoan demo can them (1-500): ");
    if (!isAllDigits(s) || s.size() > 3 || atoi(s.c_str()) < 1 || atoi(s.c_str()) > 500)
    {
        baoLoi("Vui long nhap so nguyen tu 1 den 500.");
        return;
    }
    int n = atoi(s.c_str());

    HashTable demo;
    demo.setLogRehash(true);
    cout << "Bang demo moi: capacity = " << demo.getCapacity() << ", size = 0\n";
    for (int i = 0; i < n; ++i)
    {
        Account acc(str(900001 + i), "Demo", "0900000000", "demo@demo.com", 0.0, true, getCurrentDate());
        demo.insert(acc);
    }
    cout << "\n";
    demo.displayStatistics();
    cout << "(Bang demo se bi huy; " << accounts.getSize() << " tai khoan that khong bi thay doi.)\n";
}

// ============================================================
// MENU CHINH
// ============================================================

// ============================================================
// DANG NHAP / DANG XUAT


