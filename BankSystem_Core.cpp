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

// === BankSystem_Core.cpp ===

BankSystem::BankSystem() : accounts(), db(), lastTxId(0), lastInterestMonth(""),
    vaiTro(VaiTro::KHONG), currentUser(nullptr), lastYeuCauId(0)
{
}

Transaction BankSystem::taoGiaoDichBoNho(TransactionType loai, double soTien, double soDuTruoc,
                                         double soDuSau, const string& lienQuan, const string& noiDung)
{
    ++lastTxId;
    return Transaction(maGiaoDich(lastTxId), loai, soTien, soDuTruoc, soDuSau,
                       lienQuan, lamSach(noiDung), getCurrentDateTime());
}

// Moi thay doi so du / giao dich deu di qua ham nay:
//   1. Tinh san ma giao dich + ban ghi giao dich (chua dong vao bo nho)
//   2. BEGIN -> ghi tat ca vao CSDL -> COMMIT  (loi o bat ky buoc nao -> ROLLBACK, bo nho khong doi)
//   3. Chi khi COMMIT thanh cong moi cap nhat Hash Table / linked list
// => bo nho va CSDL khong bao gio lech nhau, chuyen khoan khong the "tru A ma chua cong B".
bool BankSystem::thucThi(const vector<Buoc>& cacBuoc, const string& hanhDong, const string& chiTiet,
                         Account* taiKhoanMoi, const string& metaKey, const string& metaValue)
{
    lock_guard<mutex> lock(opsMutex);  // multi-user: mot giao dich CSDL + RAM tai mot thoi diem
    int id = lastTxId;
    string now = getCurrentDateTime();
    vector<Transaction> txs;
    // soDuTruoc dung: neu nhieu buoc tren cung 1 TK, lay soDuMoi cua buoc truoc
    for (size_t i = 0; i < cacBuoc.size(); ++i)
    {
        const Buoc& b = cacBuoc[i];
        double soDuTruoc = b.acc->getSoDu();
        for (size_t j = 0; j < i; ++j)
            if (cacBuoc[j].acc == b.acc)
                soDuTruoc = cacBuoc[j].soDuMoi;
        ++id;
        txs.push_back(Transaction(maGiaoDich(id), b.loai, b.soTien, soDuTruoc,
                                  round2(b.soDuMoi), b.lienQuan, lamSach(b.noiDung), now, b.maGoc));
    }

    if (db.isOpen())
    {
        string soTK;                                    // tai khoan ghi trong nhat ky ("" = nhieu tai khoan)
        if (taiKhoanMoi != nullptr)
            soTK = taiKhoanMoi->getSoTaiKhoan();
        else if (!cacBuoc.empty() && hanhDong != "TINH_LAI")
            soTK = cacBuoc[0].acc->getSoTaiKhoan();
        bool ok = db.begin();
        if (ok && taiKhoanMoi != nullptr) ok = db.saveAccount(*taiKhoanMoi);
        for (size_t i = 0; ok && i < cacBuoc.size(); ++i)
            ok = db.updateBalance(cacBuoc[i].acc->getSoTaiKhoan(), round2(cacBuoc[i].soDuMoi)) &&
                 db.insertTransaction(cacBuoc[i].acc->getSoTaiKhoan(), txs[i]);
        if (ok && !cacBuoc.empty()) ok = db.setMeta("last_tx_id", str(id));
        if (ok && !metaKey.empty()) ok = db.setMeta(metaKey, metaValue);
        if (ok) ok = db.audit(hanhDong, soTK, chiTiet);
        if (ok) ok = db.commit();
        if (!ok)
        {
            string e = db.lastError();
            db.rollback();
            baoLoi("Loi co so du lieu: " + e + ". Thao tac da duoc HOAN LAI, du lieu khong thay doi.");
            return false;
        }
    }

    for (size_t i = 0; i < cacBuoc.size(); ++i)
    {
        cacBuoc[i].acc->setSoDu(cacBuoc[i].soDuMoi);
        cacBuoc[i].acc->addTransaction(txs[i]);
    }
    lastTxId = id;
    return true;
}

// Luu thong tin mot tai khoan (khong dong vao so du) + ghi nhat ky
bool BankSystem::luuTaiKhoan(const Account& acc, const string& hanhDong, const string& chiTiet)
{
    lock_guard<mutex> lock(opsMutex);
    if (!db.isOpen()) return true;
    bool ok = db.begin() && db.saveAccount(acc) &&
              db.audit(hanhDong, acc.getSoTaiKhoan(), chiTiet) && db.commit();
    if (!ok)
    {
        string e = db.lastError();
        db.rollback();
        baoLoi("Loi co so du lieu: " + e + ". Thao tac da duoc HOAN LAI.");
    }
    return ok;
}

bool BankSystem::dongBoCSDL()
{
    if (!db.isOpen()) return true;
    if (!db.saveAll(accounts, lastTxId) || !db.setMeta("initialized", "1"))
    {
        baoLoi("Khong ghi duoc vao CSDL: " + db.lastError());
        return false;
    }
    return true;
}


bool BankSystem::docPinMoi(string& pin)
{
    while (true)
    {
        string a = docDong("Nhap PIN moi (4-6 chu so, 0 de huy): ");
        if (a == "0")
        {
            cout << "Đã hủy thao tác.\n";
            return false;
        }
        if (!isValidPin(a))
        {
            baoLoi("PIN phai gom 4-6 chu so.");
            continue;
        }
        string b = docDong("Nhap lai PIN: ");
        if (a != b)
        {
            baoLoi("PIN nhap lai khong khop.");
            continue;
        }
        pin = a;
        return true;
    }
}

bool BankSystem::datPin(Account* acc, const string& hanhDong)
{
    string pin;
    if (!docPinMoi(pin)) return false;

    Account cu(*acc);
    string salt = generateSalt();
    acc->setPin(salt, hashPin(pin, salt));
    acc->setSoLanSaiPin(0);
    if (!luuTaiKhoan(*acc, hanhDong, ""))
    {
        *acc = cu;
        return false;
    }
    cout << "[OK] Da dat PIN cho tai khoan " << acc->getSoTaiKhoan() << ".\n";
    return true;
}

// Xac thuc PIN. Sai MAX_PIN_TRIES lan -> khoa PIN (luu ca vao CSDL, khoi dong lai van bi khoa).
bool BankSystem::xacThucPin(Account* acc)
{
    if (acc->getSoLanSaiPin() >= MAX_PIN_TRIES)
    {
        baoLoi("PIN cua tai khoan da bi khoa do nhap sai " + str(MAX_PIN_TRIES) +
               " lan. Hay nho nhan vien mo khoa (menu 17).");
        return false;
    }
    if (!acc->coPin())
    {
        cout << "Tai khoan chua dat PIN. Can dat PIN truoc khi thuc hien thao tac nay.\n";
        return datPin(acc, "DAT_PIN");
    }

    while (acc->getSoLanSaiPin() < MAX_PIN_TRIES)
    {
        string pin = docDong("Nhap PIN (Enter de huy): ");
        if (pin.empty())
        {
            cout << "Đã hủy thao tác.\n";
            return false;
        }
        if (hashPin(pin, acc->getPinSalt()) == acc->getPinHash())
        {
            if (acc->getSoLanSaiPin() > 0)
            {
                acc->setSoLanSaiPin(0);
                luuTaiKhoan(*acc, "PIN_DUNG", "reset so lan sai");
            }
            return true;
        }
        acc->setSoLanSaiPin(acc->getSoLanSaiPin() + 1);
        luuTaiKhoan(*acc, "PIN_SAI", "lan thu " + str(acc->getSoLanSaiPin()));
        int con = MAX_PIN_TRIES - acc->getSoLanSaiPin();
        if (con > 0)
            baoLoi("Sai PIN. Con " + str(con) + " lan thu.");
        else
            baoLoi("Sai PIN " + str(MAX_PIN_TRIES) + " lan. PIN da bi KHOA.");
    }
    return false;
}

// Tong tien da rut + chuyen di trong ngay hom nay (tinh tu lich su giao dich)
double BankSystem::tongDaChiHomNay(const Account* acc) const
{
    int today = todayKey();
    double total = 0;
    for (TransactionNode* cur = acc->getTransactionHead(); cur != nullptr; cur = cur->next)
        if (laTienRa(cur->data.getLoai()) && dateKey(cur->data.getThoiGian()) == today)
            total += cur->data.getSoTien();
    return round2(total);
}

bool BankSystem::kiemTraHanMuc(const Account* acc, double soTien) const
{
    double daChi = tongDaChiHomNay(acc);
    double conLai = acc->getHanMucNgay() - daChi;
    if (soTien > conLai + 0.0001)
    {
        baoLoi("Vuot han muc giao dich trong ngay. Han muc: " + formatMoney(acc->getHanMucNgay()) +
               " | Da dung hom nay: " + formatMoney(daChi) +
               " | Con lai: " + formatMoney(conLai > 0 ? conLai : 0) + " VND.");
        return false;
    }
    return true;
}


Account* BankSystem::layTaiKhoan(const string& prompt)
{
    string soTK = docDong(prompt);
    if (soTK.empty())
    {
        baoLoi("So tai khoan khong duoc de trong.");
        return nullptr;
    }
    Account* acc = accounts.search(soTK);       // tim bang Hash Table, O(1) trung binh
    if (acc == nullptr)
        baoLoi("Tai khoan " + soTK + " khong ton tai.");
    return acc;
}

// Nhap so tai khoan, kiem tra ton tai va dang hoat dong
Account* BankSystem::layTaiKhoanHoatDong(const string& prompt)
{
    Account* acc = layTaiKhoan(prompt);
    if (acc != nullptr && !acc->getTrangThai())
    {
        baoLoi("Tai khoan " + acc->getSoTaiKhoan() + " da dong, khong the thuc hien giao dich.");
        return nullptr;
    }
    return acc;
}

// Khach hang: dung tai khoan dang dang nhap. Admin: nhap so TK nhu cu.
Account* BankSystem::layTaiKhoanTheoPhien(const string& prompt)
{
    if (vaiTro == VaiTro::KHACH)
    {
        if (currentUser == nullptr)
        {
            baoLoi("Phien dang nhap khong hop le. Hay dang nhap lai.");
            return nullptr;
        }
        if (!currentUser->getTrangThai())
        {
            baoLoi("Tai khoan da dong, khong the thuc hien giao dich.");
            return nullptr;
        }
        return currentUser;
    }
    return layTaiKhoanHoatDong(prompt);
}

// Nhap so tien. Nhap sai thi bao loi va nhap lai.
// Neu khong cho phep 0 thi nhap "0" nghia la HUY (tra false).
bool BankSystem::docSoTien(const string& prompt, double& out, bool choPhepKhong)
{
    while (true)
    {
        string s = docDong(prompt);
        if (!choPhepKhong && s == "0")
        {
            cout << "Đã hủy thao tác.\n";
            return false;
        }

        string loi;
        if (!phanTichSoTien(s, out, loi))
        {
            baoLoi(loi);
            continue;
        }
        if (!choPhepKhong && out <= 0)
        {
            baoLoi("So tien phai lon hon 0.");
            continue;
        }
        return true;
    }
}

bool BankSystem::xacNhan(const string& cauHoi)
{
    while (true)
    {
        string s = docDong(cauHoi + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        baoLoi("Vui long nhap y hoac n.");
    }
}

void BankSystem::tamDung()
{
    cout << "\n";
    docDong("Nhấn Enter để quay lại menu...");
}


void BankSystem::taoTaiKhoanMau(const string& soTK, const string& hoTen, const string& sdt,
                                const string& email, double soDuBanDau)
{
    Account acc(soTK, hoTen, sdt, email, 0.0, true, getCurrentDate());
    string salt = generateSalt();
    acc.setPin(salt, hashPin(PIN_MAU, salt));
    if (soDuBanDau > 0)
    {
        acc.addTransaction(taoGiaoDichBoNho(NAP_TIEN, soDuBanDau, 0.0, soDuBanDau, "", "Nap tien ban dau"));
        acc.setSoDu(soDuBanDau);
    }
    accounts.insert(acc);
}

void BankSystem::taoDuLieuMau()
{
    accounts.clear();
    lastTxId = 0;

    cout << "Tao 5 tai khoan mau (ban demo)...\n";
    cout << "So TK duoc sinh ngau nhien (cung co che nhu mo TK moi):\n";

    struct Mau { const char* ten; const char* sdt; const char* email; double soDu; };
    Mau ds[] = {
        {"Nguyen Van An",  "0901111111", "an@gmail.com",    10000000},
        {"Tran Van Binh",  "0902222222", "binh@gmail.com",   5000000},
        {"Le Van Cuong",   "0903333333", "cuong@gmail.com", 20000000},
        {"Pham Van Dung",  "0904444444", "dung@gmail.com",   8000000},
        {"Hoang Van Em",   "0905555555", "em@gmail.com",    15000000}
    };

    for (int i = 0; i < 5; ++i)
    {
        string soTK = sinhSoTaiKhoanNgauNhien(accounts);
        taoTaiKhoanMau(soTK, ds[i].ten, ds[i].sdt, ds[i].email, ds[i].soDu);
        cout << "  - " << soTK << " | " << ds[i].ten
             << " | " << formatMoney(ds[i].soDu) << " VND\n";
    }
    cout << "\nDa tao TK mau. Thong tin PIN/Admin kiem thu: xem README (chi dung cho demo).\n";
}

void BankSystem::khoiTao()
{
    accounts.setLogRehash(false);       // tat thong bao rehash khi nap du lieu luc khoi dong

    bool coDb = db.open(DB_FILE);
    if (!coDb)
    {
        if (Database::available())
            cout << "[CANH BAO] Khong mo duoc CSDL " << DB_FILE << ": " << db.lastError()
                 << "\n           Chuyen sang che do file .txt.\n";
        else
            cout << "[THONG BAO] Ban bien dich nay khong kem SQLite -> luu du lieu bang file .txt.\n";
    }
    else if (!db.loadAll(accounts, lastTxId))
    {
        cout << "[CANH BAO] Khong doc duoc CSDL: " << db.lastError() << "\n           Chuyen sang che do file .txt.\n";
        db.close();
        coDb = false;
    }

    if (coDb)
    {
        string daKhoiTao;
        if (db.getMeta("initialized", daKhoiTao))
        {
            cout << "Da doc du lieu tu CSDL SQLite (" << DB_FILE << "): "
                 << accounts.getSize() << " tai khoan, " << lastTxId << " ma giao dich da cap.\n";
        }
        else
        {
            if (FileManager::loadAll(accounts, lastTxId, ACCOUNT_FILE, TRANSACTION_FILE))
                cout << "Da DI TRU du lieu tu file .txt vao CSDL: " << accounts.getSize() << " tai khoan.\n";
            else
            {
                taoDuLieuMau();
                cout << "CSDL moi. Da tao " << accounts.getSize() << " tai khoan mau (demo).\n";
            }
            if (!dongBoCSDL())
            {
                cout << "[CANH BAO] Khong ghi duoc CSDL, chuyen sang che do file .txt.\n";
                db.close();
            }
        }
        if (db.isOpen())
            db.getMeta("last_interest_month", lastInterestMonth);
    }
    else
    {
        if (FileManager::loadAll(accounts, lastTxId, ACCOUNT_FILE, TRANSACTION_FILE))
            cout << "Da doc du lieu tu file: " << accounts.getSize() << " tai khoan.\n";
        else
        {
            taoDuLieuMau();
            cout << "Khong tim thay file du lieu. Da tao " << accounts.getSize()
                 << " tai khoan mau (demo).\n";
        }
    }

    
    if (db.isOpen())
    {
        vector<string> ma, gui, tra, nd, tt, tg;
        vector<double> st;
        if (db.loadMoneyRequests(ma, gui, tra, st, nd, tt, tg))
        {
            dsYeuCau.clear();
            int maxId = 0;
            for (size_t i = 0; i < ma.size(); ++i)
            {
                YeuCauTien yc;
                yc.ma = ma[i]; yc.soTkGui = gui[i]; yc.soTkTra = tra[i];
                yc.soTien = st[i]; yc.noiDung = nd[i]; yc.trangThai = tt[i]; yc.thoiGian = tg[i];
                dsYeuCau.push_back(yc);
                if (ma[i].size() > 2 && ma[i].substr(0, 2) == "YC")
                {
                    string d = ma[i].substr(2);
                    if (isAllDigits(d)) maxId = max(maxId, atoi(d.c_str()));
                }
            }
            if (maxId > lastYeuCauId) lastYeuCauId = maxId;
            if (!dsYeuCau.empty())
                cout << "Da nap " << dsYeuCau.size() << " yeu cau nhan tien tu CSDL.\n";
        }
    }

accounts.setLogRehash(true);
}


