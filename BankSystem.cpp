#include "BankSystem.h"
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

// ============================================================
// Hang so va ham ho tro nhap lieu / hien thi (chi dung trong file nay)
// ============================================================
namespace
{
    const string DB_FILE = "bank.db";
    const string ACCOUNT_FILE = "accounts.txt";
    const string TRANSACTION_FILE = "transactions.txt";
    const double MAX_AMOUNT = 1e12;      // toi da moi giao dich
    const double MAX_BALANCE = 1e15;     // toi da so du mot tai khoan
    const int MAX_PIN_TRIES = 3;         // sai PIN qua so lan nay -> khoa PIN
    const string PIN_MAU = "123456";     // PIN cua cac tai khoan mau
    const double PHI_RUT_TIEN = 1100.0;  // phi rut tien mat (VND)
    const double PHI_CHUYEN_KHOAN = 1100.0; // phi sau khi het luot mien phi
    const int SO_LAN_MIEN_PHI_CHUYEN = 5;     // N lan chuyen mien phi / thang
    const double LAI_HU_TIET_KIEM = 0.3;       // %/thang cho hu (minh hoa)

    // Tai khoan quan tri (bai tap: hardcode; production nen luu hash trong CSDL)
    const string ADMIN_USER = "admin";
    const string ADMIN_PASS = "admin123";

    string str(long long v)
    {
        ostringstream oss;
        oss << v;
        return oss.str();
    }

    string maGiaoDich(int id)
    {
        ostringstream oss;
        oss << "GD" << setfill('0') << setw(6) << id;
        return oss.str();
    }

    // Doc mot dong tu ban phim, bo khoang trang thua
    string docDong(const string& prompt)
    {
        UI::prompt(prompt);
        string line;
        if (!getline(cin, line))
            throw InputClosed();
        return trim(line);
    }

    void inTieuDe(const string& title)
    {
        UI::title(title);
    }

    void baoLoi(const string& msg)
    {
        UI::error(msg);
    }

    // Ky tu '|' dung de phan cach trong file nen doi thanh '/'
    string lamSach(string s)
    {
        for (size_t i = 0; i < s.size(); ++i)
            if (s[i] == '|') s[i] = '/';
        return s;
    }

    bool hopLeSoTaiKhoan(const string& s)
    {
        return isAllDigits(s) && s.size() >= 6 && s.size() <= 15;
    }

    bool hopLeSoDienThoai(const string& s)
    {
        return isAllDigits(s) && s.size() >= 9 && s.size() <= 11;
    }

    bool hopLeHoTen(const string& s)
    {
        if (s.empty() || s.size() > 50) return false;
        for (size_t i = 0; i < s.size(); ++i)
            if (s[i] == '|' || isdigit((unsigned char)s[i])) return false;
        return true;
    }

    bool hopLeEmail(const string& e)
    {
        if (e.empty() || e.size() > 60) return false;
        for (size_t i = 0; i < e.size(); ++i)
            if (isspace((unsigned char)e[i]) || e[i] == '|') return false;
        size_t at = e.find('@');
        if (at == string::npos || at == 0) return false;
        if (e.find('@', at + 1) != string::npos) return false;
        size_t dot = e.rfind('.');
        return dot != string::npos && dot > at + 1 && dot + 1 < e.size();
    }

    // Phan tich so tien: chi chu so, toi da 1 dau '.', toi da 2 chu so thap phan
    bool phanTichSoTien(const string& s, double& out, string& loi)
    {
        if (s.empty()) { loi = "Khong duoc de trong."; return false; }
        if (s[0] == '-') { loi = "So tien khong duoc am."; return false; }
        if (s.size() > 16) { loi = "So tien qua lon."; return false; }

        size_t dot = string::npos;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '.')
            {
                if (dot != string::npos) { loi = "Dinh dang so tien khong hop le."; return false; }
                dot = i;
            }
            else if (!isdigit((unsigned char)s[i]))
            {
                loi = "Chi duoc nhap chu so (vi du: 500000 hoac 1500.50).";
                return false;
            }
        }
        if (dot == 0 || dot == s.size() - 1) { loi = "Dinh dang so tien khong hop le."; return false; }
        if (dot != string::npos && s.size() - dot - 1 > 2) { loi = "Toi da 2 chu so thap phan."; return false; }

        out = strtod(s.c_str(), nullptr);
        if (out > MAX_AMOUNT) { loi = "So tien vuot gioi han 1,000,000,000,000 VND."; return false; }
        return true;
    }

    bool laTienRa(TransactionType t)
    {
        return t == RUT_TIEN || t == CHUYEN_KHOAN || t == PHI_GIAO_DICH
            || t == THANH_TOAN_HD || t == NAP_DT || t == GUI_TIET_KIEM;
    }

    // yyyymm hien tai
    int thangHienTaiKey()
    {
        string d = getCurrentDate(); // dd/mm/yyyy
        if (d.size() < 10) return 0;
        int m = atoi(d.substr(3, 2).c_str());
        int y = atoi(d.substr(6, 4).c_str());
        return y * 100 + m;
    }

    // Tinh phi chuyen: mien phi N lan/thang
    double tinhPhiChuyen(Account* a)
    {
        int tk = thangHienTaiKey();
        if (a->getThangChuyenKey() != tk)
        {
            a->setThangChuyenKey(tk);
            a->setSoLanChuyenThang(0);
        }
        if (a->getSoLanChuyenThang() < SO_LAN_MIEN_PHI_CHUYEN)
            return 0.0;
        return PHI_CHUYEN_KHOAN;
    }

    void tangSoLanChuyen(Account* a)
    {
        int tk = thangHienTaiKey();
        if (a->getThangChuyenKey() != tk)
        {
            a->setThangChuyenKey(tk);
            a->setSoLanChuyenThang(0);
        }
        a->setSoLanChuyenThang(a->getSoLanChuyenThang() + 1);
    }


    bool hopLeCmnd(const string& s)
    {
        return isAllDigits(s) && (s.size() == 9 || s.size() == 12);
    }

    bool hopLeNgaySinh(const string& s)
    {
        int key = 0;
        if (!parseDateInput(s, key)) return false;
        // nam sinh hop ly: 1900 .. nam hien tai - 15
        int y = key / 10000;
        int todayY = todayKey() / 10000;
        return y >= 1900 && y <= todayY - 15;
    }

    // Lay toi da maxN thong bao nhan tien gan nhat cua 1 tai khoan
    vector<string> layThongBaoNhanTien(const Account* acc, int maxN = 5)
    {
        vector<string> all;
        if (acc == nullptr) return all;
        for (TransactionNode* cur = acc->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            const Transaction& t = cur->data;
            if (t.getLoai() != NHAN_CHUYEN_KHOAN) continue;
            string from = t.getTaiKhoanLienQuan().empty() ? "?" : t.getTaiKhoanLienQuan();
            string line = "+" + formatMoney(t.getSoTien()) + " VND tu TK " + from
                        + " | " + t.getThoiGian();
            if (!t.getNoiDung().empty())
                line += " | " + t.getNoiDung();
            all.push_back(line);
        }
        // lay cuoi danh sach (giao dich moi nhat o cuoi linked list)
        vector<string> out;
        int start = (int)all.size() > maxN ? (int)all.size() - maxN : 0;
        for (int i = start; i < (int)all.size(); ++i)
            out.push_back(all[i]);
        // hien moi nhat tren cung
        reverse(out.begin(), out.end());
        return out;
    }


    // In bang giao dich (dung cho sao ke, ket qua loc)
    void inBangGiaoDich(const vector<const Transaction*>& ds)
    {
        if (ds.empty())
        {
            cout << "Khong co giao dich nao.\n";
            return;
        }
        cout << left
             << setw(10) << "Ma GD" << "| "
             << setw(18) << "Loai GD" << "| "
             << setw(18) << "So tien" << "| "
             << setw(18) << "So du sau" << "| "
             << setw(20) << "Thoi gian" << "| "
             << setw(13) << "TK lien quan" << "| "
             << "Noi dung" << "\n";
        cout << string(125, '-') << "\n";

        double vao = 0, ra = 0;
        for (size_t i = 0; i < ds.size(); ++i)
        {
            const Transaction& t = *ds[i];
            bool tienRa = laTienRa(t.getLoai());
            string tien = (tienRa ? "-" : "+") + formatMoney(t.getSoTien());
            if (tienRa) ra += t.getSoTien(); else vao += t.getSoTien();

            cout << left
                 << setw(10) << t.getMaGiaoDich() << "| "
                 << setw(18) << Transaction::typeToString(t.getLoai()) << "| "
                 << right << setw(18) << tien << left << "| "
                 << right << setw(18) << formatMoney(t.getSoDuSau()) << left << "| "
                 << setw(20) << t.getThoiGian() << "| "
                 << setw(13) << (t.getTaiKhoanLienQuan().empty() ? "-" : t.getTaiKhoanLienQuan()) << "| "
                 << t.getNoiDung() << "\n";
        }
        cout << string(125, '-') << "\n";
        cout << "So giao dich: " << ds.size()
             << " | Tong tien vao: +" << formatMoney(vao)
             << " | Tong tien ra: -" << formatMoney(ra) << " VND\n";
    }

    void inChiTietGiaoDich(const string& soTK, const string& chuTK, const Transaction& t)
    {
        cout << "  Ma giao dich  : " << t.getMaGiaoDich() << "\n";
        cout << "  Tai khoan     : " << soTK << " (" << chuTK << ")\n";
        cout << "  Loai          : " << Transaction::typeToString(t.getLoai()) << "\n";
        cout << "  So tien       : " << (laTienRa(t.getLoai()) ? "-" : "+") << formatMoney(t.getSoTien()) << " VND\n";
        cout << "  So du truoc   : " << formatMoney(t.getSoDuTruoc()) << " VND\n";
        cout << "  So du sau     : " << formatMoney(t.getSoDuSau()) << " VND\n";
        cout << "  TK lien quan  : " << (t.getTaiKhoanLienQuan().empty() ? "-" : t.getTaiKhoanLienQuan()) << "\n";
        cout << "  Noi dung      : " << t.getNoiDung() << "\n";
        cout << "  Thoi gian     : " << t.getThoiGian() << "\n";
    }

    // In bang danh sach tai khoan
    void inBangTaiKhoan(const vector<Account*>& ds)
    {
        cout << left
             << setw(5) << "STT" << "| "
             << setw(16) << "So tai khoan" << "| "
             << setw(22) << "Ho ten" << "| "
             << setw(14) << "So dien thoai" << "| "
             << setw(24) << "Email" << "| "
             << setw(18) << "So du (VND)" << "| "
             << setw(15) << "Trang thai" << "| "
             << "Ngay mo\n";
        cout << string(135, '-') << "\n";

        for (size_t i = 0; i < ds.size(); ++i)
        {
            const Account* a = ds[i];
            string tt = a->getTrangThai() ? "Hoat dong" : "Da dong";
            if (a->getSoLanSaiPin() >= MAX_PIN_TRIES) tt += " (khoa PIN)";
            cout << left
                 << setw(5) << (i + 1) << "| "
                 << setw(16) << a->getSoTaiKhoan() << "| "
                 << setw(22) << a->getHoTen() << "| "
                 << setw(14) << a->getSoDienThoai() << "| "
                 << setw(24) << a->getEmail() << "| "
                 << right << setw(18) << formatMoney(a->getSoDu()) << left << "| "
                 << setw(15) << tt << "| "
                 << a->getNgayMo() << "\n";
        }
        cout << string(135, '-') << "\n";
    }

    bool xuatCsvSaoKe(const string& file, const vector<const Transaction*>& ds)
    {
        ofstream out(file.c_str());
        if (!out) return false;
        out << fixed << setprecision(2);
        out << "ma_gd,loai,so_tien,so_du_truoc,so_du_sau,tk_lien_quan,noi_dung,thoi_gian\n";
        for (size_t i = 0; i < ds.size(); ++i)
        {
            const Transaction& t = *ds[i];
            out << csvEscape(t.getMaGiaoDich()) << ","
                << csvEscape(Transaction::typeToString(t.getLoai())) << ","
                << t.getSoTien() << "," << t.getSoDuTruoc() << "," << t.getSoDuSau() << ","
                << csvEscape(t.getTaiKhoanLienQuan()) << ","
                << csvEscape(t.getNoiDung()) << ","
                << csvEscape(t.getThoiGian()) << "\n";
        }
        return out.good();
    }

    vector<const Transaction*> layGiaoDich(const Account& acc)
    {
        vector<const Transaction*> ds;
        for (TransactionNode* cur = acc.getTransactionHead(); cur != nullptr; cur = cur->next)
            ds.push_back(&cur->data);
        return ds;
    }
}

// ============================================================
// Constructor, loi ghi du lieu
// ============================================================

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

// ============================================================
// Bao mat: PIN, han muc
// ============================================================

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

// ============================================================
// Ham ho tro nhap lieu
// ============================================================

// Nhap so tai khoan, kiem tra ton tai (ke ca tai khoan da dong)
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

// ============================================================
// Khoi tao va du lieu mau
// ============================================================

// Tao tai khoan mau (chi trong bo nho; sau do dongBoCSDL() ghi xuong CSDL)
// Sinh so tai khoan 10 chu so ngau nhien, dam bao khong trung trong he thong.
// Dinh dang: 1xxxxxxxxx (bat dau bang 1, giong so TK ngan hang).
static string sinhSoTaiKhoanNgauNhien(const HashTable& accounts)
{
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<long long> dist(1000000000LL, 1999999999LL);  // 10 chu so, bat dau bang 1

    for (int thu = 0; thu < 1000; ++thu)
    {
        ostringstream oss;
        oss << dist(gen);
        string soTK = oss.str();
        // Kiem tra dinh dang hop le + khong trung
        if (hopLeSoTaiKhoan(soTK) && accounts.search(soTK) == nullptr)
            return soTK;
    }
    // Du phong rat hiem: dung timestamp
    ostringstream oss;
    oss << "1" << (time(nullptr) % 1000000000LL);
    string soTK = oss.str();
    while (soTK.size() < 10) soTK += "0";
    if (soTK.size() > 10) soTK = soTK.substr(0, 10);
    if (!hopLeSoTaiKhoan(soTK)) soTK = "1000000001";
    return soTK;
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

    
    // Nap yeu cau nhan tien tu CSDL (neu co)
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
                yc.ma = ma[i];
                yc.soTkGui = gui[i];
                yc.soTkTra = tra[i];
                yc.soTien = st[i];
                yc.noiDung = nd[i];
                yc.trangThai = tt[i];
                yc.thoiGian = tg[i];
                dsYeuCau.push_back(yc);
                // YC000012 -> 12
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

// ============================================================
// 1. MO TAI KHOAN
// ============================================================


void BankSystem::moTaiKhoan()
{
    inTieuDe("MỞ TÀI KHOẢN MỚI");

    // 1. He thong tu cap so tai khoan ngau nhien (khong cho user nhap)
    string soTK = sinhSoTaiKhoanNgauNhien(accounts);
    cout << "So tai khoan duoc cap: " << soTK << "\n";
    cout << "(He thong tu dong sinh, khong can nhap tay.)\n\n";

    // 3. Ho ten
    string hoTen;
    while (true)
    {
        hoTen = docDong("Nhap ho ten: ");
        if (hopLeHoTen(hoTen)) break;
        baoLoi("Ho ten khong duoc rong, khong chua chu so hoac ky tu '|' (toi da 50 ky tu).");
    }

    // 4. So dien thoai
    string sdt;
    while (true)
    {
        sdt = docDong("Nhap so dien thoai: ");
        if (hopLeSoDienThoai(sdt)) break;
        baoLoi("So dien thoai phai gom 9-11 chu so.");
    }

    // 5. Email
    string email;
    while (true)
    {
        email = docDong("Nhap email: ");
        if (hopLeEmail(email)) break;
        baoLoi("Email khong hop le (vi du: ten@gmail.com).");
    }

    // 5b. CMND/CCCD (KYC)
    string cmnd;
    while (true)
    {
        cmnd = docDong("Nhap CMND/CCCD (9 hoac 12 chu so): ");
        if (hopLeCmnd(cmnd)) break;
        baoLoi("CMND/CCCD phai gom 9 hoac 12 chu so.");
    }

    // 5c. Ngay sinh
    string ngaySinh;
    while (true)
    {
        ngaySinh = docDong("Nhap ngay sinh (dd/mm/yyyy): ");
        if (hopLeNgaySinh(ngaySinh)) break;
        baoLoi("Ngay sinh khong hop le (dd/mm/yyyy, tuoi >= 15).");
    }

    // 5d. Loai tai khoan
    LoaiTaiKhoan loai = THANH_TOAN;
    while (true)
    {
        cout << "Chon loai tai khoan:\n";
        cout << "  1. Thanh toan (lai " << LAI_THANH_TOAN << "%/thang)\n";
        cout << "  2. Tiet kiem  (lai " << LAI_TIET_KIEM << "%/thang)\n";
        string ch = docDong("Nhap lua chon (1/2): ");
        if (ch == "1") { loai = THANH_TOAN; break; }
        if (ch == "2") { loai = TIET_KIEM; break; }
        baoLoi("Vui long chon 1 hoac 2.");
    }

    // 6. PIN
    cout << "Dat ma PIN cho tai khoan (dung de xac thuc khi rut tien / chuyen khoan).\n";
    string pin;
    if (!docPinMoi(pin)) return;

    // 7. So tien ban dau
    double soDuBanDau;
    docSoTien("Nhap so tien gui ban dau (VND, co the nhap 0): ", soDuBanDau, true);

    // 8. Tao tai khoan: dua vao Hash Table roi ghi CSDL (tai khoan + giao dich nap dau) trong 1 giao dich
    Account acc(soTK, hoTen, sdt, email, 0.0, true, getCurrentDate(), loai, cmnd, ngaySinh);
    string salt = generateSalt();
    acc.setPin(salt, hashPin(pin, salt));
    if (!accounts.insert(acc))
    {
        baoLoi("Khong the them tai khoan (trung so tai khoan).");
        return;
    }
    Account* pAcc = accounts.search(soTK);

    vector<Buoc> cacBuoc;
    if (soDuBanDau > 0)
    {
        Buoc b = {pAcc, soDuBanDau, NAP_TIEN, soDuBanDau, "", "Nap tien ban dau khi mo tai khoan", ""};
        cacBuoc.push_back(b);
    }
    if (!thucThi(cacBuoc, "MO_TK", hoTen + ", nap dau " + formatMoney(soDuBanDau), pAcc))
    {
        accounts.remove(soTK);          // CSDL that bai -> go khoi bo nho de khong lech
        return;
    }

    cout << "\n[OK] Mo tai khoan thanh cong!\n";
    pAcc->display();
    cout << "Tai khoan duoc luu o bucket " << accounts.hashFunction(soTK)
         << " (capacity hien tai = " << accounts.getCapacity() << ").\n";
}

// ============================================================
// 2. DONG TAI KHOAN
// ============================================================
// Co 2 cach "dong": (1) Khoa - doi trang thai, giu lich su; (2) Xoa vinh vien - Delete trong Hash Table + CSDL.
// Dieu kien: so du phai bang 0.
void BankSystem::dongTaiKhoan()
{
    inTieuDe("ĐÓNG TÀI KHOẢN");

    Account* acc = layTaiKhoan("Nhap so tai khoan can dong: ");
    if (acc == nullptr) return;
    string soTK = acc->getSoTaiKhoan();

    acc->display();

    if (acc->getSoDu() > 0)
    {
        baoLoi("Tai khoan con so du " + formatMoney(acc->getSoDu()) +
               " VND. Hay rut tien hoac chuyen khoan het truoc khi dong.");
        return;
    }

    cout << "\nChon cach xu ly:\n";
    cout << "1. Khoa tai khoan (doi trang thai thanh 'Da dong', giu lich su)\n";
    cout << "2. Xoa vinh vien khoi he thong (Hash Table + CSDL)\n";
    cout << "0. Huy\n";
    string choice = docDong("Nhập lựa chọn: ");

    if (choice == "1")
    {
        if (!acc->getTrangThai())
        {
            baoLoi("Tai khoan nay da duoc dong truoc do.");
            return;
        }
        if (!xacNhan("Xac nhan khoa tai khoan " + soTK))
        {
            cout << "Đã hủy thao tác.\n";
            return;
        }
        acc->setTrangThai(false);
        if (!luuTaiKhoan(*acc, "DONG_TK", "khoa tai khoan"))
        {
            acc->setTrangThai(true);
            return;
        }
        cout << "[OK] Da dong tai khoan " << soTK << " (van co the xem sao ke, co the mo lai o menu 6).\n";
    }
    else if (choice == "2")
    {
        if (!xacNhan("Xac nhan XOA VINH VIEN tai khoan " + soTK + " va toan bo lich su giao dich"))
        {
            cout << "Đã hủy thao tác.\n";
            return;
        }
        if (db.isOpen())
        {
            bool ok = db.begin() && db.deleteAccount(soTK) &&
                      db.audit("XOA_TK", soTK, "xoa vinh vien, " + str(acc->getTransactionCount()) + " giao dich") &&
                      db.commit();
            if (!ok)
            {
                string e = db.lastError();
                db.rollback();
                baoLoi("Loi co so du lieu: " + e + ". Tai khoan khong bi xoa.");
                return;
            }
        }
        if (accounts.remove(soTK))
            cout << "[OK] Da xoa tai khoan " << soTK << ". So tai khoan con lai: " << accounts.getSize() << "\n";
        else
            baoLoi("Khong the xoa tai khoan.");
    }
    else
    {
        cout << "Đã hủy thao tác.\n";
    }
}

// ============================================================
// 6. MO LAI TAI KHOAN DA DONG
// ============================================================
void BankSystem::moLaiTaiKhoan()
{
    inTieuDe("MO LAI TAI KHOAN DA DONG");

    Account* acc = layTaiKhoan("Nhap so tai khoan: ");
    if (acc == nullptr) return;
    if (acc->getTrangThai())
    {
        baoLoi("Tai khoan nay dang hoat dong, khong can mo lai.");
        return;
    }
    acc->display();
    if (!xacNhan("Xac nhan mo lai tai khoan " + acc->getSoTaiKhoan()))
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }
    acc->setTrangThai(true);
    if (!luuTaiKhoan(*acc, "MO_LAI_TK", ""))
    {
        acc->setTrangThai(false);
        return;
    }
    cout << "[OK] Da mo lai tai khoan " << acc->getSoTaiKhoan() << ".\n";
}

// ============================================================
// 3. TIM KIEM TAI KHOAN (so tai khoan / ten / sdt / email)
// ============================================================
void BankSystem::timTaiKhoan()
{
    inTieuDe("TÌM KIẾM TÀI KHOẢN");
    cout << "1. Theo so tai khoan (tra cuu Hash Table, O(1))\n";
    cout << "2. Theo ho ten (chua tu khoa, khong phan biet hoa thuong)\n";
    cout << "3. Theo so dien thoai (chua tu khoa)\n";
    cout << "4. Theo email (chua tu khoa)\n";
    cout << "0. Huy\n";
    string choice = docDong("Nhập lựa chọn: ");

    if (choice == "1")
    {
        string soTK = docDong("Nhap so tai khoan can tim: ");
        if (soTK.empty())
        {
            baoLoi("So tai khoan khong duoc de trong.");
            return;
        }
        Account* acc = accounts.search(soTK);
        if (acc == nullptr)
        {
            baoLoi("Khong tim thay tai khoan " + soTK + ".");
            return;
        }
        cout << "\n[Qua trinh tim kiem] key = " << soTK
             << " -> hashFunction = bucket " << accounts.hashFunction(soTK)
             << " -> vi tri thu " << accounts.getChainPosition(soTK)
             << " trong chuoi (do dai chuoi = "
             << accounts.getBucketLength(accounts.hashFunction(soTK)) << ")\n\n";
        acc->display();
    }
    else if (choice == "2" || choice == "3" || choice == "4")
    {
        string tuKhoa = docDong("Nhap tu khoa: ");
        if (tuKhoa.empty())
        {
            baoLoi("Tu khoa khong duoc de trong.");
            return;
        }
        vector<Account*> all = accounts.getAllAccounts();
        vector<Account*> kq;
        for (size_t i = 0; i < all.size(); ++i)
        {
            string giaTri = (choice == "2") ? all[i]->getHoTen()
                          : (choice == "3") ? all[i]->getSoDienThoai()
                                            : all[i]->getEmail();
            if (containsIgnoreCase(giaTri, tuKhoa)) kq.push_back(all[i]);
        }
        cout << "\n(Quet toan bo " << all.size() << " tai khoan - O(n), vi Hash Table chi toi uu cho tim theo key)\n";
        if (kq.empty())
            cout << "Khong tim thay tai khoan nao khop voi \"" << tuKhoa << "\".\n";
        else
        {
            inBangTaiKhoan(kq);
            cout << "Tim thay " << kq.size() << " tai khoan.\n";
        }
    }
    else
    {
        cout << "Đã hủy thao tác.\n";
    }
}

// ============================================================
// 4. HIEN THI THONG TIN TAI KHOAN
// ============================================================
void BankSystem::hienThiTaiKhoan()
{
    inTieuDe("HIỂN THỊ THÔNG TIN TÀI KHOẢN");

    Account* acc = (vaiTro == VaiTro::KHACH) ? currentUser : layTaiKhoan("Nhap so tai khoan: ");
    if (acc == nullptr) return;

    acc->display();
    cout << "So giao dich da thuc hien: " << acc->getTransactionCount() << "\n";
    cout << "Da chi hom nay (rut + chuyen): " << formatMoney(tongDaChiHomNay(acc))
         << " / " << formatMoney(acc->getHanMucNgay()) << " VND\n";
}

// ============================================================
// 5. CAP NHAT THONG TIN KHACH HANG
// ============================================================
void BankSystem::capNhatThongTin()
{
    inTieuDe("CẬP NHẬT THÔNG TIN KHÁCH HÀNG");

    Account* acc = (vaiTro == VaiTro::KHACH) ? currentUser : layTaiKhoan("Nhap so tai khoan: ");
    if (acc == nullptr) return;
    acc->display();

    cout << "\n1. Doi ho ten\n2. Doi so dien thoai\n3. Doi email\n0. Huy\n";
    string choice = docDong("Nhập lựa chọn: ");
    if (choice != "1" && choice != "2" && choice != "3")
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }

    string moi;
    while (true)
    {
        if (choice == "1")
        {
            moi = docDong("Nhap ho ten moi (0 de huy): ");
            if (moi == "0") { cout << "Đã hủy thao tác.\n"; return; }
            if (hopLeHoTen(moi)) break;
            baoLoi("Ho ten khong duoc rong, khong chua chu so hoac ky tu '|' (toi da 50 ky tu).");
        }
        else if (choice == "2")
        {
            moi = docDong("Nhap so dien thoai moi (0 de huy): ");
            if (moi == "0") { cout << "Đã hủy thao tác.\n"; return; }
            if (hopLeSoDienThoai(moi)) break;
            baoLoi("So dien thoai phai gom 9-11 chu so.");
        }
        else
        {
            moi = docDong("Nhap email moi (0 de huy): ");
            if (moi == "0") { cout << "Đã hủy thao tác.\n"; return; }
            if (hopLeEmail(moi)) break;
            baoLoi("Email khong hop le (vi du: ten@gmail.com).");
        }
    }

    if (!xacThucPin(acc)) return;

    Account cu(*acc);
    string truong;
    if (choice == "1") { acc->setHoTen(moi); truong = "ho ten"; }
    else if (choice == "2") { acc->setSoDienThoai(moi); truong = "so dien thoai"; }
    else { acc->setEmail(moi); truong = "email"; }

    if (!luuTaiKhoan(*acc, "CAP_NHAT_TT", truong + " -> " + moi))
    {
        *acc = cu;
        return;
    }
    cout << "[OK] Da cap nhat " << truong << ".\n";
}

// ============================================================
// 7. NAP TIEN
// ============================================================
void BankSystem::napTien()
{
    inTieuDe("NẠP TIỀN");
    // Admin (thu ngan) nap ho; Khach hang nap vao TK cua minh
    Account* acc = layTaiKhoanTheoPhien("Nhap so tai khoan can nap: ");
    if (acc == nullptr) return;

    cout << "Chu tai khoan: " << acc->getHoTen()
         << " | So du hien tai: " << formatMoney(acc->getSoDu()) << " VND\n";

    double soTien;
    if (!docSoTien("Nhap so tien nap (VND, 0 de huy): ", soTien, false)) return;

    if (acc->getSoDu() + soTien > MAX_BALANCE)
    {
        baoLoi("So du sau khi nap vuot gioi han cho phep.");
        return;
    }

    vector<Buoc> cacBuoc;
    Buoc b = {acc, acc->getSoDu() + soTien, NAP_TIEN, soTien, "", "Nap tien mat", ""};
    cacBuoc.push_back(b);
    if (!thucThi(cacBuoc, "NAP_TIEN", acc->getSoTaiKhoan() + " +" + formatMoney(soTien))) return;

    cout << "[OK] Nap tien thanh cong. So du moi: " << formatMoney(acc->getSoDu()) << " VND\n";
}

// ============================================================
// 8. RUT TIEN
// ============================================================
void BankSystem::rutTien()
{
    inTieuDe("RÚT TIỀN");
    if (vaiTro == VaiTro::ADMIN)
    {
        baoLoi("Tai khoan Admin khong duoc rut tien. Hay dang nhap bang TK khach hang.");
        return;
    }
    Account* acc = layTaiKhoanTheoPhien("Nhap so tai khoan: ");
    if (acc == nullptr) return;

    cout << "Chu tai khoan: " << acc->getHoTen()
         << " | So TK: " << acc->getSoTaiKhoan()
         << " | So du hien tai: " << formatMoney(acc->getSoDu()) << " VND\n";
    cout << "Han muc con lai hom nay: "
         << formatMoney(max(0.0, acc->getHanMucNgay() - tongDaChiHomNay(acc))) << " VND\n";

    double soTien;
    if (!docSoTien("Nhap so tien rut (VND, 0 de huy): ", soTien, false)) return;

    double phi = PHI_RUT_TIEN;
    double tongTru = soTien + phi;
    if (acc->getSoDu() < tongTru)
    {
        baoLoi("So du khong du (can " + formatMoney(tongTru) + " = " + formatMoney(soTien) +
               " + phi " + formatMoney(phi) + "). So du hien tai: " + formatMoney(acc->getSoDu()) + " VND.");
        return;
    }
    if (!kiemTraHanMuc(acc, soTien)) return;  // han muc chi tinh so tien rut, khong tinh phi
    if (!xacThucPin(acc)) return;

    if (phi > 0)
        cout << "Phi rut tien: " << formatMoney(phi) << " VND. Tong tru: " << formatMoney(tongTru) << " VND.\n";

    vector<Buoc> cacBuoc;
    double duSauRut = acc->getSoDu() - soTien;
    Buoc b1 = {acc, duSauRut, RUT_TIEN, soTien, "", "Rut tien mat", ""};
    cacBuoc.push_back(b1);
    if (phi > 0)
    {
        Buoc b2 = {acc, duSauRut - phi, PHI_GIAO_DICH, phi, "", "Phi rut tien", ""};
        cacBuoc.push_back(b2);
    }
    if (!thucThi(cacBuoc, "RUT_TIEN", acc->getSoTaiKhoan() + " -" + formatMoney(soTien) +
                 (phi > 0 ? " (phi " + formatMoney(phi) + ")" : ""))) return;

    cout << "[OK] Rut tien thanh cong. So du moi: " << formatMoney(acc->getSoDu()) << " VND\n";
}

// ============================================================
// 9. CHUYEN KHOAN
// ============================================================
// Kiem tra TAT CA dieu kien truoc; sau do 2 ben duoc ghi trong CUNG MOT giao dich CSDL
// (hoac ca hai thanh cong, hoac ca hai khong doi) -> khong the tru tien A ma chua cong B.
void BankSystem::chuyenKhoan()
{
    inTieuDe("CHUYỂN KHOẢN");
    if (vaiTro == VaiTro::ADMIN)
    {
        baoLoi("Tai khoan Admin khong duoc chuyen khoan. Hay dang nhap bang TK khach hang.");
        return;
    }
    Account* a = layTaiKhoanTheoPhien("Nhap tai khoan nguon: ");
    if (a == nullptr) return;
    if (vaiTro == VaiTro::KHACH)
        cout << "(Tai khoan nguon: " << a->getSoTaiKhoan() << " - " << a->getHoTen() << ")\n";
    cout << "Chu tai khoan nguon: " << a->getHoTen()
         << " | So du: " << formatMoney(a->getSoDu()) << " VND\n";

    Account* b = layTaiKhoanHoatDong("Nhap tai khoan giao dich: ");
    if (b == nullptr) return;

    if (a == b)
    {
        baoLoi("Khong the chuyen khoan cho chinh minh.");
        return;
    }
    cout << "Chu tai khoan dich : " << b->getHoTen() << "\n";
    cout << "Han muc con lai hom nay: "
         << formatMoney(max(0.0, a->getHanMucNgay() - tongDaChiHomNay(a))) << " VND\n";

    double soTien;
    if (!docSoTien("Nhap so tien chuyen (VND, 0 de huy): ", soTien, false)) return;

    double phi = tinhPhiChuyen(a);
    cout << "Phi: " << (phi < 0.01 ? "MIEN PHI" : formatMoney(phi) + " VND")
         << " (con " << max(0, SO_LAN_MIEN_PHI_CHUYEN - a->getSoLanChuyenThang())
         << " lan mien phi thang nay)\n";
    double tongTru = soTien + phi;
    if (a->getSoDu() < tongTru)
    {
        baoLoi("So du khong du (can " + formatMoney(tongTru) + "). So du hien tai: " +
               formatMoney(a->getSoDu()) + " VND.");
        return;
    }
    if (!kiemTraHanMuc(a, soTien)) return;
    if (b->getSoDu() + soTien > MAX_BALANCE)
    {
        baoLoi("So du tai khoan dich se vuot gioi han cho phep.");
        return;
    }

    string noiDung = docDong("Nhap noi dung chuyen khoan (Enter de bo trong): ");
    if (noiDung.empty())
        noiDung = "Chuyen khoan";

    string xacNhanMsg = "Xac nhan chuyen " + formatMoney(soTien) + " VND tu " +
                        a->getSoTaiKhoan() + " den " + b->getSoTaiKhoan();
    if (phi > 0)
        xacNhanMsg += " (phi " + formatMoney(phi) + " VND)";
    if (!xacNhan(xacNhanMsg))
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }
    if (!xacThucPin(a)) return;

    vector<Buoc> cacBuoc;
    double duSauA = a->getSoDu() - soTien;
    Buoc bA = {a, duSauA, CHUYEN_KHOAN, soTien, b->getSoTaiKhoan(), noiDung, ""};
    Buoc bB = {b, b->getSoDu() + soTien, NHAN_CHUYEN_KHOAN, soTien, a->getSoTaiKhoan(), noiDung, ""};
    cacBuoc.push_back(bA);
    cacBuoc.push_back(bB);
    if (phi > 0)
    {
        Buoc bPhi = {a, duSauA - phi, PHI_GIAO_DICH, phi, "", "Phi chuyen khoan", ""};
        cacBuoc.push_back(bPhi);
    }
    if (!thucThi(cacBuoc, "CHUYEN_KHOAN",
                 a->getSoTaiKhoan() + " -> " + b->getSoTaiKhoan() + " " + formatMoney(soTien)))
        return;

    {
        vector<pair<string, string> > rows;
        rows.push_back(make_pair("Tu tai khoan", a->getSoTaiKhoan() + " (" + a->getHoTen() + ")"));
        rows.push_back(make_pair("Den tai khoan", b->getSoTaiKhoan() + " (" + b->getHoTen() + ")"));
        rows.push_back(make_pair("So tien", formatMoney(soTien) + " VND"));
        if (phi > 0)
            rows.push_back(make_pair("Phi", formatMoney(phi) + " VND"));
        rows.push_back(make_pair("Noi dung", noiDung));
        rows.push_back(make_pair("So du con lai", formatMoney(a->getSoDu()) + " VND"));
        rows.push_back(make_pair("Trang thai", "THANH CONG"));
        tangSoLanChuyen(a);
        luuTaiKhoan(*a, "CAP_NHAT_SO_LAN_CHUYEN", "");
        UI::receipt("CHUYEN KHOAN THANH CONG", rows);
        UI::success("Tien da duoc gui den TK " + b->getSoTaiKhoan() + ". Nguoi nhan se thay trong thong bao.");
    }
}

// ============================================================
// 10. XEM SAO KE (co loc) + XUAT CSV
// ============================================================
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
// 12. HIEN THI TAT CA TAI KHOAN
// ============================================================
void BankSystem::hienThiTatCa()
{
    inTieuDe("DANH SÁCH TẤT CẢ TÀI KHOẢN");

    vector<Account*> all = accounts.getAllAccounts();
    if (all.empty())
    {
        cout << "Chua co tai khoan nao.\n";
        return;
    }
    inBangTaiKhoan(all);
    cout << "Tong cong: " << all.size() << " tai khoan\n";
}

// ============================================================
// 13. THONG KE HE THONG
// ============================================================
void BankSystem::thongKe()
{
    inTieuDe("THỐNG KÊ HỆ THỐNG");

    vector<Account*> all = accounts.getAllAccounts();

    int active = 0, closed = 0, coPin = 0, khoaPin = 0;
    int soNap = 0, soRut = 0, soChuyen = 0, soNhan = 0, soLai = 0, soHomNay = 0;
    double tongSoDu = 0, tongNap = 0, tongRut = 0, tongChuyen = 0, tongLai = 0;
    const Account* giauNhat = nullptr;
    int today = todayKey();

    for (size_t i = 0; i < all.size(); ++i)
    {
        const Account* a = all[i];
        if (a->getTrangThai()) ++active; else ++closed;
        if (a->coPin()) ++coPin;
        if (a->getSoLanSaiPin() >= MAX_PIN_TRIES) ++khoaPin;
        tongSoDu += a->getSoDu();
        if (giauNhat == nullptr || a->getSoDu() > giauNhat->getSoDu())
            giauNhat = a;

        for (TransactionNode* cur = a->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            const Transaction& t = cur->data;
            if (dateKey(t.getThoiGian()) == today) ++soHomNay;
            switch (t.getLoai())
            {
            case NAP_TIEN:          ++soNap;    tongNap += t.getSoTien();    break;
            case RUT_TIEN:          ++soRut;    tongRut += t.getSoTien();    break;
            case CHUYEN_KHOAN:      ++soChuyen; tongChuyen += t.getSoTien(); break;
            case NHAN_CHUYEN_KHOAN: ++soNhan;   break;
            case TRA_LAI:           ++soLai;    tongLai += t.getSoTien();    break;
            case PHI_GIAO_DICH:     ++soRut;    tongRut += t.getSoTien();    break;
            case DAO_GIAO_DICH:     break;
            case THANH_TOAN_HD:     ++soRut;    tongRut += t.getSoTien();    break;
            case NAP_DT:            ++soRut;    tongRut += t.getSoTien();    break;
            case GUI_TIET_KIEM:     break;
            case RUT_TIET_KIEM:     break;
            }
        }
    }

    cout << "Tong so tai khoan      : " << all.size() << "\n";
    cout << "  - Dang hoat dong     : " << active << "\n";
    cout << "  - Da dong            : " << closed << "\n";
    cout << "  - Da dat PIN         : " << coPin << " (bi khoa PIN: " << khoaPin << ")\n";
    cout << "Tong so du he thong    : " << formatMoney(tongSoDu) << " VND\n";
    if (all.size() > 0)
        cout << "So du trung binh       : " << formatMoney(tongSoDu / (double)all.size()) << " VND\n";
    if (giauNhat != nullptr)
        cout << "So du cao nhat         : " << formatMoney(giauNhat->getSoDu())
             << " VND (TK " << giauNhat->getSoTaiKhoan() << " - " << giauNhat->getHoTen() << ")\n";
    cout << "Tong so giao dich      : " << (soNap + soRut + soChuyen + soNhan + soLai)
         << " (hom nay: " << soHomNay << ")\n";
    cout << "  - Nap tien           : " << soNap << " | " << formatMoney(tongNap) << " VND\n";
    cout << "  - Rut tien           : " << soRut << " | " << formatMoney(tongRut) << " VND\n";
    cout << "  - Chuyen khoan       : " << soChuyen << " | " << formatMoney(tongChuyen) << " VND\n";
    cout << "  - Nhan chuyen khoan  : " << soNhan << "\n";
    cout << "  - Tra lai            : " << soLai << " | " << formatMoney(tongLai) << " VND\n";
    cout << "Lan tinh lai gan nhat  : " << (lastInterestMonth.empty() ? "chua tinh" : lastInterestMonth) << "\n";
    cout << "Ma giao dich da cap    : " << lastTxId << "\n";
    cout << "Hash Table             : capacity = " << accounts.getCapacity()
         << ", size = " << accounts.getSize()
         << ", load factor = " << fixed << setprecision(4) << accounts.getLoadFactor() << "\n";
    cout.unsetf(ios::floatfield);

    if (db.isOpen())
        cout << "Co so du lieu          : SQLite " << db.version() << " | " << db.path()
             << " | " << db.count("accounts") << " TK, " << db.count("transactions") << " GD, "
             << db.count("audit_log") << " dong nhat ky\n";
    else
        cout << "Co so du lieu          : khong su dung (che do file .txt)\n";
}

// ============================================================
// 14. TOP TAI KHOAN CO SO DU CAO NHAT
// ============================================================
void BankSystem::topTaiKhoan()
{
    inTieuDe("TOP TÀI KHOẢN SỐ DƯ CAO NHẤT");

    string s = docDong("Nhap N (1-50, Enter = 5): ");
    int n = 5;
    if (!s.empty())
    {
        if (!isAllDigits(s) || s.size() > 2 || atoi(s.c_str()) < 1 || atoi(s.c_str()) > 50)
        {
            baoLoi("N phai la so nguyen tu 1 den 50.");
            return;
        }
        n = atoi(s.c_str());
    }

    vector<Account*> all = accounts.getAllAccounts();
    stable_sort(all.begin(), all.end(), [](const Account* x, const Account* y)
                { return x->getSoDu() > y->getSoDu(); });
    if ((int)all.size() > n) all.resize((size_t)n);

    if (all.empty())
    {
        cout << "Chua co tai khoan nao.\n";
        return;
    }
    inBangTaiKhoan(all);
}

// ============================================================
// 15. TINH LAI HANG THANG
// ============================================================
void BankSystem::tinhLai()
{
    inTieuDe("TÍNH LÃI HÀNG THÁNG");

    string thang = getCurrentDate().substr(3);      // "mm/yyyy"
    if (lastInterestMonth == thang)
    {
        cout << "Canh bao: thang " << thang << " da duoc tinh lai roi.\n";
        if (!xacNhan("Van tiep tuc tinh lai lan nua?"))
        {
            cout << "Đã hủy thao tác.\n";
            return;
        }
    }

    cout << "Che do tinh lai:\n";
    cout << "  1. Theo loai TK (Thanh toan " << LAI_THANH_TOAN << "% / Tiet kiem " << LAI_TIET_KIEM << "%)\n";
    cout << "  2. Nhap lai suat chung cho tat ca\n";
    cout << "  0. Huy\n";
    string mode = docDong("Nhap lua chon: ");
    if (mode == "0")
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }

    double rateChung = -1;
    if (mode == "2")
    {
        string s = docDong("Nhap lai suat thang (%, vi du 0.5; toi da 5; 0 de huy): ");
        if (s == "0")
        {
            cout << "Đã hủy thao tác.\n";
            return;
        }
        string loi;
        if (!phanTichSoTien(s, rateChung, loi))
        {
            baoLoi(loi);
            return;
        }
        if (rateChung <= 0 || rateChung > 5)
        {
            baoLoi("Lai suat phai lon hon 0 va khong qua 5%/thang.");
            return;
        }
    }
    else if (mode != "1")
    {
        baoLoi("Lua chon khong hop le.");
        return;
    }

    vector<Account*> all = accounts.getAllAccounts();
    vector<Buoc> cacBuoc;
    double tongLai = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        Account* a = all[i];
        if (!a->getTrangThai() || a->getSoDu() <= 0) continue;
        double rate = (rateChung > 0) ? rateChung : a->getLaiSuatThang();
        double lai = round2(a->getSoDu() * rate / 100.0);
        if (lai <= 0 || a->getSoDu() + lai > MAX_BALANCE) continue;
        ostringstream rs;
        rs << rate;
        string nd = "Lai suat thang " + thang + " (" + rs.str() + "% - " +
                    Account::loaiToString(a->getLoaiTaiKhoan()) + ")";
        Buoc b = {a, a->getSoDu() + lai, TRA_LAI, lai, "", nd, ""};
        cacBuoc.push_back(b);
        tongLai += lai;
    }

    if (cacBuoc.empty())
    {
        cout << "Khong co tai khoan nao du dieu kien nhan lai.\n";
        return;
    }
    cout << "Se tra lai cho " << cacBuoc.size() << " tai khoan, tong "
         << formatMoney(tongLai) << " VND.\n";
    if (!xacNhan("Xac nhan tra lai"))
    {
        cout << "Đã hủy thao tác.\n";
        return;
    }

    if (!thucThi(cacBuoc, "TINH_LAI", thang + " | " + str((long long)cacBuoc.size()) +
                 " TK | " + formatMoney(tongLai) + " VND", nullptr, "last_interest_month", thang))
        return;
    lastInterestMonth = thang;
    cout << "[OK] Da tra lai cho " << cacBuoc.size() << " tai khoan (" << formatMoney(tongLai) << " VND).\n";
}

// ============================================================
// 11b. DAO / HOAN GIAO DICH
// ============================================================
// Chi dao duoc: NAP_TIEN, RUT_TIEN, CHUYEN_KHOAN (+ NHAN_CHUYEN_KHOAN doi ung),
// PHI_GIAO_DICH. Khong dao TRA_LAI / DAO_GIAO_DICH. Moi ma GD chi dao 1 lan.
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
// 16. DAT / DOI PIN
// ============================================================
void BankSystem::datDoiPin()
{
    inTieuDe("ĐẶT / ĐỔI PIN");

    Account* acc = layTaiKhoanTheoPhien("Nhap so tai khoan: ");
    if (acc == nullptr) return;

    if (!acc->coPin())
    {
        cout << "Tai khoan chua co PIN. Dat PIN moi.\n";
        datPin(acc, "DAT_PIN");
        return;
    }
    cout << "Nhap PIN hien tai de xac thuc.\n";
    if (!xacThucPin(acc)) return;
    datPin(acc, "DOI_PIN");
}

// ============================================================
// 17. MO KHOA PIN / DAT LAI PIN (nhan vien)
// ============================================================
void BankSystem::moKhoaPin()
{
    inTieuDe("MO KHOA PIN / DAT LAI PIN");

    Account* acc = layTaiKhoan("Nhap so tai khoan: ");
    if (acc == nullptr) return;

    cout << "Trang thai PIN: "
         << (!acc->coPin() ? "chua dat" : (acc->getSoLanSaiPin() >= MAX_PIN_TRIES ? "BI KHOA" : "binh thuong"))
         << " | So lan sai: " << acc->getSoLanSaiPin() << "/" << MAX_PIN_TRIES << "\n";
    cout << "1. Mo khoa (xoa so lan sai, giu nguyen PIN)\n";
    cout << "2. Dat lai PIN moi (khach quen PIN)\n";
    cout << "0. Huy\n";
    string choice = docDong("Nhập lựa chọn: ");

    if (choice == "1")
    {
        if (acc->getSoLanSaiPin() == 0)
        {
            cout << "PIN dang binh thuong, khong can mo khoa.\n";
            return;
        }
        if (!xacNhan("Xac nhan nhan vien da kiem tra giay to va mo khoa PIN"))
        {
            cout << "Đã hủy thao tác.\n";
            return;
        }
        Account cu(*acc);
        acc->setSoLanSaiPin(0);
        if (!luuTaiKhoan(*acc, "MO_KHOA_PIN", ""))
        {
            *acc = cu;
            return;
        }
        cout << "[OK] Da mo khoa PIN.\n";
    }
    else if (choice == "2")
    {
        if (!xacNhan("Xac nhan nhan vien da kiem tra giay to va dat lai PIN"))
        {
            cout << "Đã hủy thao tác.\n";
            return;
        }
        datPin(acc, "RESET_PIN");
    }
    else
    {
        cout << "Đã hủy thao tác.\n";
    }
}

// ============================================================
// 18. DOI HAN MUC GIAO DICH / NGAY
// ============================================================
void BankSystem::doiHanMuc()
{
    inTieuDe("ĐỔI HẠN MỨC GIAO DỊCH / NGÀY");

    Account* acc = layTaiKhoanTheoPhien("Nhap so tai khoan: ");
    if (acc == nullptr) return;

    cout << "Han muc hien tai: " << formatMoney(acc->getHanMucNgay()) << " VND/ngay"
         << " | Da dung hom nay: " << formatMoney(tongDaChiHomNay(acc)) << " VND\n";

    double moi;
    if (!docSoTien("Nhap han muc moi (VND/ngay, 0 de huy): ", moi, false)) return;
    if (!xacThucPin(acc)) return;

    double cu = acc->getHanMucNgay();
    acc->setHanMucNgay(moi);
    if (!luuTaiKhoan(*acc, "DOI_HAN_MUC", formatMoney(cu) + " -> " + formatMoney(moi)))
    {
        acc->setHanMucNgay(cu);
        return;
    }
    cout << "[OK] Da dat han muc moi: " << formatMoney(acc->getHanMucNgay()) << " VND/ngay\n";
}

// ============================================================
// 19. NHAT KY HE THONG
// ============================================================
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
                cout << "[OK] Da tao lai " << accounts.getSize() << " tai khoan mau (demo).\n";
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
// ============================================================

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
        UI::info("Ban demo hoc tap: xem README de biet thong tin dang nhap kiem thu.");
    }
    else
        UI::info("Chua co tai khoan. Dang nhap Admin de mo TK moi.");

    cout << "\n";
}

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

void BankSystem::chuyenTheoSdt()
{
    inTieuDe("CHUYỂN TIỀN THEO SỐ ĐIỆN THOẠI");
    if (vaiTro == VaiTro::ADMIN)
    {
        baoLoi("Admin khong duoc chuyen tien.");
        return;
    }
    Account* a = layTaiKhoanTheoPhien();
    if (a == nullptr) return;

    string sdt = docDong("Nhap so dien thoai nguoi nhan: ");
    if (!hopLeSoDienThoai(sdt))
    {
        baoLoi("So dien thoai khong hop le (9-11 chu so).");
        return;
    }

    vector<Account*> all = accounts.getAllAccounts();
    vector<Account*> match;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i]->getSoDienThoai() == sdt && all[i]->getTrangThai()
            && all[i]->getSoTaiKhoan() != a->getSoTaiKhoan())
            match.push_back(all[i]);
    }
    if (match.empty())
    {
        baoLoi("Khong tim thay tai khoan hoat dong voi SĐT " + sdt);
        return;
    }

    Account* b = match[0];
    if (match.size() > 1)
    {
        cout << "Tim thay " << match.size() << " TK cung SĐT:\n";
        for (size_t i = 0; i < match.size(); ++i)
            cout << "  " << (i + 1) << ". " << match[i]->getSoTaiKhoan()
                 << " - " << match[i]->getHoTen() << "\n";
        string ch = docDong("Chon STT: ");
        int idx = atoi(ch.c_str()) - 1;
        if (idx < 0 || idx >= (int)match.size())
        {
            baoLoi("Lua chon khong hop le.");
            return;
        }
        b = match[idx];
    }

    cout << "Nguoi nhan: " << b->getHoTen() << " | TK " << b->getSoTaiKhoan() << "\n";
    double soTien = 0;
    if (!docSoTien("Nhap so tien chuyen: ", soTien, false)) return;
    if (soTien < 100)
    {
        baoLoi("So tien toi thieu 100 VND (kieu MoMo).");
        return;
    }
    if (!kiemTraHanMuc(a, soTien)) return;

    double phi = tinhPhiChuyen(a);
    double tong = soTien + phi;
    if (a->getSoDu() < tong)
    {
        baoLoi("So du khong du (can " + formatMoney(tong) + " gom phi " + formatMoney(phi) + ").");
        return;
    }
    string noiDung = docDong("Loi nhan (Enter = trong): ");
    if (noiDung.empty()) noiDung = "Chuyen qua SĐT " + sdt;

    cout << "Phi chuyen: " << (phi < 0.01 ? "MIEN PHI" : formatMoney(phi) + " VND");
    cout << " (con " << max(0, SO_LAN_MIEN_PHI_CHUYEN - a->getSoLanChuyenThang())
         << " lan mien phi trong thang)\n";
    if (!xacNhan("Xac nhan chuyen " + formatMoney(soTien) + " den " + b->getHoTen())) return;
    if (!xacThucPin(a)) return;

    vector<Buoc> buoc;
    double sa = a->getSoDu();
    double sb = b->getSoDu();
    buoc.push_back({a, sa - soTien, CHUYEN_KHOAN, soTien, b->getSoTaiKhoan(), noiDung, ""});
    buoc.push_back({b, sb + soTien, NHAN_CHUYEN_KHOAN, soTien, a->getSoTaiKhoan(), noiDung, ""});
    if (phi > 0)
        buoc.push_back({a, sa - soTien - phi, PHI_GIAO_DICH, phi, "", "Phi chuyen khoan", ""});

    if (!thucThi(buoc, "CHUYEN_SDT", a->getSoTaiKhoan() + "->" + b->getSoTaiKhoan()))
        return;

    tangSoLanChuyen(a);
    luuTaiKhoan(*a, "CAP_NHAT_SO_LAN_CHUYEN", "");
    {
        vector<pair<string, string> > rows;
        rows.push_back(make_pair("Tu", a->getSoTaiKhoan() + " (" + a->getHoTen() + ")"));
        rows.push_back(make_pair("Den SĐT", sdt + " / TK " + b->getSoTaiKhoan()));
        rows.push_back(make_pair("Nguoi nhan", b->getHoTen()));
        rows.push_back(make_pair("So tien", formatMoney(soTien) + " VND"));
        rows.push_back(make_pair("Phi", phi < 0.01 ? "Mien phi" : formatMoney(phi) + " VND"));
        rows.push_back(make_pair("Noi dung", noiDung));
        UI::receipt("CHUYEN THEO SĐT THANH CONG", rows);
    }
}

void BankSystem::taoYeuCauTien()
{
    inTieuDe("YÊU CẦU NHẬN TIỀN (ĐÒI TIỀN)");
    if (vaiTro != VaiTro::KHACH || currentUser == nullptr)
    {
        baoLoi("Chi khach hang moi tao yeu cau nhan tien.");
        return;
    }
    Account* a = currentUser;
    string soTk = docDong("Nhap so TK nguoi can doi tien: ");
    Account* b = accounts.search(soTk);
    if (b == nullptr || !b->getTrangThai())
    {
        baoLoi("Tai khoan khong ton tai hoac da dong.");
        return;
    }
    if (b->getSoTaiKhoan() == a->getSoTaiKhoan())
    {
        baoLoi("Khong the doi chinh minh.");
        return;
    }
    double soTien = 0;
    if (!docSoTien("Nhap so tien yeu cau: ", soTien, false)) return;
    string nd = docDong("Ly do / noi dung: ");
    if (nd.empty()) nd = "Yeu cau thanh toan";

    YeuCauTien yc;
    ++lastYeuCauId;
    ostringstream oss;
    oss << "YC" << setfill('0') << setw(6) << lastYeuCauId;
    yc.ma = oss.str();
    yc.soTkGui = a->getSoTaiKhoan();
    yc.soTkTra = b->getSoTaiKhoan();
    yc.soTien = soTien;
    yc.noiDung = nd;
    yc.trangThai = "CHO";
    yc.thoiGian = getCurrentDateTime();
    dsYeuCau.push_back(yc);

    if (db.isOpen())
    {
        if (!db.saveMoneyRequest(yc.ma, yc.soTkGui, yc.soTkTra, yc.soTien, yc.noiDung, yc.trangThai, yc.thoiGian))
            baoLoi("Canh bao: khong luu duoc yeu cau vao CSDL: " + db.lastError());
        else
            db.setMeta("last_yeucau_id", str(lastYeuCauId));
        db.audit("YEU_CAU_TIEN", a->getSoTaiKhoan(), yc.ma + " doi " + b->getSoTaiKhoan() + " " + formatMoney(soTien));
    }

    UI::success("Da gui yeu cau " + yc.ma + " den TK " + b->getSoTaiKhoan());
    cout << "  Nguoi kia se thay trong menu 'Xu ly yeu cau tien'.\n";
}

void BankSystem::xuLyYeuCauTien()
{
    inTieuDe("XỬ LÝ YÊU CẦU NHẬN TIỀN");
    if (vaiTro != VaiTro::KHACH || currentUser == nullptr)
    {
        baoLoi("Chi khach hang moi xu ly yeu cau.");
        return;
    }
    string me = currentUser->getSoTaiKhoan();
    vector<int> idx;
    for (size_t i = 0; i < dsYeuCau.size(); ++i)
    {
        if (dsYeuCau[i].soTkTra == me && dsYeuCau[i].trangThai == "CHO")
            idx.push_back((int)i);
        if (dsYeuCau[i].soTkGui == me)
            cout << "  [Da tao] " << dsYeuCau[i].ma << " | doi " << dsYeuCau[i].soTkTra
                 << " | " << formatMoney(dsYeuCau[i].soTien) << " | " << dsYeuCau[i].trangThai << "\n";
    }
    if (idx.empty())
    {
        UI::info("Khong co yeu cau CHO xu ly (ban la ben tra tien).");
        return;
    }
    cout << "\nYeu cau dang CHO ban tra:\n";
    for (size_t k = 0; k < idx.size(); ++k)
    {
        const YeuCauTien& y = dsYeuCau[idx[k]];
        cout << "  " << (k + 1) << ". " << y.ma << " | tu TK " << y.soTkGui
             << " | " << formatMoney(y.soTien) << " VND | " << y.noiDung << "\n";
    }
    string ch = docDong("Chon STT (0 huy): ");
    int st = atoi(ch.c_str());
    if (st <= 0 || st > (int)idx.size()) return;
    YeuCauTien& y = dsYeuCau[idx[st - 1]];

    vector<string> opts;
    opts.push_back("1. Dong y thanh toan");
    opts.push_back("2. Tu choi");
    opts.push_back("0. Bo qua");
    int sel = UI::selectMenu("Xu ly " + y.ma, opts, 0);
    if (sel == 2 || sel < 0) return;
    if (sel == 1)
    {
        y.trangThai = "TU_CHOI";
        if (db.isOpen()) { db.updateMoneyRequestStatus(y.ma, "TU_CHOI"); db.audit("TU_CHOI_YEU_CAU", me, y.ma); }
        UI::info("Da tu choi yeu cau " + y.ma);
        return;
    }

    Account* a = currentUser;
    Account* b = accounts.search(y.soTkGui);
    if (b == nullptr)
    {
        baoLoi("TK nguoi doi khong con ton tai.");
        return;
    }
    if (a->getSoDu() < y.soTien)
    {
        baoLoi("So du khong du de thanh toan.");
        return;
    }
    if (!xacThucPin(a)) return;

    vector<Buoc> buoc;
    buoc.push_back({a, a->getSoDu() - y.soTien, CHUYEN_KHOAN, y.soTien, b->getSoTaiKhoan(),
                    "Tra yeu cau " + y.ma + ": " + y.noiDung, ""});
    buoc.push_back({b, b->getSoDu() + y.soTien, NHAN_CHUYEN_KHOAN, y.soTien, a->getSoTaiKhoan(),
                    "Nhan tu yeu cau " + y.ma, ""});
    if (!thucThi(buoc, "TRA_YEU_CAU", y.ma)) return;
    y.trangThai = "DA_TRA";
    if (db.isOpen()) db.updateMoneyRequestStatus(y.ma, "DA_TRA");
    UI::success("Da thanh toan yeu cau " + y.ma);
}

void BankSystem::thanhToanHoaDon()
{
    inTieuDe("THANH TOÁN HÓA ĐƠN (MOCK)");
    if (vaiTro == VaiTro::ADMIN) { baoLoi("Dung tai khoan khach."); return; }
    Account* a = layTaiKhoanTheoPhien();
    if (a == nullptr) return;

    vector<string> opts;
    opts.push_back("1. Dien luc EVN");
    opts.push_back("2. Nuoc sach");
    opts.push_back("3. Internet FPT");
    opts.push_back("4. Truyen hinh Viettel");
    opts.push_back("0. Huy");
    int sel = UI::selectMenu("Chon nha cung cap", opts, 0);
    if (sel < 0 || sel == 4) return;
    const char* ncc[] = {"EVN", "NUOC", "FPT", "ViettelTV"};
    string maHD = docDong("Nhap ma khach hang / ma hoa don: ");
    if (maHD.empty()) maHD = "HD-DEMO";
    double soTien = 0;
    if (!docSoTien("Nhap so tien can thanh toan: ", soTien, false)) return;
    if (a->getSoDu() < soTien)
    {
        baoLoi("So du khong du.");
        return;
    }
    if (!xacNhan("Thanh toan " + formatMoney(soTien) + " cho " + ncc[sel] + "?")) return;
    if (!xacThucPin(a)) return;

    vector<Buoc> buoc;
    buoc.push_back({a, a->getSoDu() - soTien, THANH_TOAN_HD, soTien, ncc[sel],
                    string("TT HD ") + ncc[sel] + " #" + maHD, ""});
    if (!thucThi(buoc, "THANH_TOAN_HD", ncc[sel])) return;
    UI::success("Da thanh toan hoa don " + string(ncc[sel]) + " - " + formatMoney(soTien) + " VND");
}

void BankSystem::napDienThoai()
{
    inTieuDe("NẠP ĐIỆN THOẠI (MOCK)");
    if (vaiTro == VaiTro::ADMIN) { baoLoi("Dung tai khoan khach."); return; }
    Account* a = layTaiKhoanTheoPhien();
    if (a == nullptr) return;

    string sdt = docDong("Nhap SĐT can nap (Enter = SĐT cua ban): ");
    if (sdt.empty()) sdt = a->getSoDienThoai();
    if (!hopLeSoDienThoai(sdt))
    {
        baoLoi("SĐT khong hop le.");
        return;
    }
    vector<string> opts;
    opts.push_back("1. 10,000 VND");
    opts.push_back("2. 20,000 VND");
    opts.push_back("3. 50,000 VND");
    opts.push_back("4. 100,000 VND");
    opts.push_back("5. So khac");
    opts.push_back("0. Huy");
    int sel = UI::selectMenu("Chon menh gia", opts, 0);
    if (sel < 0 || sel == 5) return;
    double soTien = 0;
    if (sel == 0) soTien = 10000;
    else if (sel == 1) soTien = 20000;
    else if (sel == 2) soTien = 50000;
    else if (sel == 3) soTien = 100000;
    else if (!docSoTien("Nhap so tien nap: ", soTien, false)) return;

    if (a->getSoDu() < soTien)
    {
        baoLoi("So du khong du.");
        return;
    }
    if (!xacNhan("Nap " + formatMoney(soTien) + " cho SĐT " + sdt + "?")) return;
    if (!xacThucPin(a)) return;

    vector<Buoc> buoc;
    buoc.push_back({a, a->getSoDu() - soTien, NAP_DT, soTien, sdt, "Nap DT " + sdt, ""});
    if (!thucThi(buoc, "NAP_DT", sdt)) return;
    UI::success("Nap dien thoai thanh cong (mock) - " + formatMoney(soTien) + " cho " + sdt);
}

void BankSystem::quanLyTietKiem()
{
    inTieuDe("HŨ TIẾT KIỆM NỘI BỘ");
    if (vaiTro != VaiTro::KHACH || currentUser == nullptr)
    {
        baoLoi("Chi khach hang su dung hu tiet kiem.");
        return;
    }
    Account* a = currentUser;
    cout << "  So du thanh toan : " << formatMoney(a->getSoDu()) << " VND\n";
    cout << "  So du hu TK      : " << formatMoney(a->getSoDuTietKiem()) << " VND\n";
    cout << "  Lai hu (minh hoa): " << LAI_HU_TIET_KIEM << "%/thang\n\n";

    vector<string> opts;
    opts.push_back("1. Gui vao hu tiet kiem");
    opts.push_back("2. Rut tu hu ra so du");
    opts.push_back("0. Quay lai");
    int sel = UI::selectMenu("Hu tiet kiem", opts, 0);
    if (sel < 0 || sel == 2) return;

    double soTien = 0;
    if (!docSoTien("Nhap so tien: ", soTien, false)) return;
    if (!xacThucPin(a)) return;

    if (sel == 0)
    {
        if (a->getSoDu() < soTien) { baoLoi("So du khong du."); return; }
        vector<Buoc> buoc;
        buoc.push_back({a, a->getSoDu() - soTien, GUI_TIET_KIEM, soTien, "", "Gui hu tiet kiem", ""});
        if (!thucThi(buoc, "GUI_TIET_KIEM", a->getSoTaiKhoan())) return;
        a->setSoDuTietKiem(a->getSoDuTietKiem() + soTien);
        luuTaiKhoan(*a, "CAP_NHAT_HU_TK", "");
        UI::success("Da gui " + formatMoney(soTien) + " vao hu. Hu = " + formatMoney(a->getSoDuTietKiem()));
    }
    else
    {
        if (a->getSoDuTietKiem() < soTien) { baoLoi("Hu khong du."); return; }
        vector<Buoc> buoc;
        buoc.push_back({a, a->getSoDu() + soTien, RUT_TIET_KIEM, soTien, "", "Rut hu tiet kiem", ""});
        if (!thucThi(buoc, "RUT_TIET_KIEM", a->getSoTaiKhoan())) return;
        a->setSoDuTietKiem(a->getSoDuTietKiem() - soTien);
        luuTaiKhoan(*a, "CAP_NHAT_HU_TK", "");
        UI::success("Da rut " + formatMoney(soTien) + " tu hu. Hu = " + formatMoney(a->getSoDuTietKiem()));
    }
}

void BankSystem::trungTamTroGiup()
{
    inTieuDe("TRUNG TÂM TRỢ GIÚP");
    vector<string> opts;
    opts.push_back("1. Huong dan chuyen tien / SĐT");
    opts.push_back("2. Phi va han muc");
    opts.push_back("3. Yeu cau nhan tien");
    opts.push_back("4. Hoa don & nap DT");
    opts.push_back("5. Hu tiet kiem");
    opts.push_back("6. Bao mat PIN");
    opts.push_back("0. Quay lai");
    int sel = UI::selectMenu("Tro giup Nexus Bank", opts, 0);
    cout << "\n";
    if (sel == 0)
    {
        UI::info("Chuyen theo STK: menu Chuyen khoan.");
        UI::info("Chuyen theo SĐT: menu Chuyen theo SĐT - he thong tim TK theo so dien thoai.");
        UI::info("Toi thieu 100 VND. Can PIN va du han muc ngay.");
    }
    else if (sel == 1)
    {
        cout << "  - Chuyen noi bo: mien phi " << SO_LAN_MIEN_PHI_CHUYEN
             << " lan/thang, sau do phi " << formatMoney(PHI_CHUYEN_KHOAN) << " VND/lan.\n";
        cout << "  - Rut tien mat: phi " << formatMoney(PHI_RUT_TIEN) << " VND/lan.\n";
        cout << "  - Han muc rut+chuyen mac dinh: " << formatMoney(MAC_DINH_HAN_MUC) << " VND/ngay.\n";
    }
    else if (sel == 2)
    {
        UI::info("Tao yeu cau: ban doi tien tu TK khac.");
        UI::info("Nguoi bi doi vao 'Xu ly yeu cau tien' de Dong y (tru tien) hoac Tu choi.");
    }
    else if (sel == 3)
    {
        UI::info("Hoa don: EVN/Nuoc/FPT/ViettelTV - chi mo phong, khong ket noi that.");
        UI::info("Nap DT: chon menh gia, tru so du TK (mock).");
    }
    else if (sel == 4)
    {
        UI::info("Hu tiet kiem: tach 1 phan so du sang 'hu' rieng trong cung TK.");
        UI::info("Gui/rut noi bo, co lai minh hoa " + string("") + ".");
        cout << "  Lai hu: " << LAI_HU_TIET_KIEM << "%/thang (chi hien thi, tinh lai that qua menu Admin).\n";
    }
    else if (sel == 5)
    {
        UI::info("PIN 4-6 so, bam SHA-256 + salt. Sai 3 lan -> khoa, can Admin mo khoa.");
        UI::info("Admin: dang nhap quan tri (xem README) - khong rut/chuyen.");
    }
}

void BankSystem::menuKhach()
{
    while (vaiTro == VaiTro::KHACH)
    {
        hienMenuKhach();
        // Thong bao yeu cau CHO
        int pending = 0;
        if (currentUser)
        {
            for (size_t i = 0; i < dsYeuCau.size(); ++i)
                if (dsYeuCau[i].soTkTra == currentUser->getSoTaiKhoan() && dsYeuCau[i].trangThai == "CHO")
                    ++pending;
            if (pending > 0)
                UI::warn("Ban co " + str(pending) + " yeu cau nhan tien dang cho xu ly!");
        }

        vector<string> opts;
        opts.push_back("1.  Xem thong tin tai khoan");
        opts.push_back("2.  Cap nhat thong tin");
        opts.push_back("3.  Nap tien");
        opts.push_back("4.  Rut tien");
        opts.push_back("5.  Chuyen khoan (theo STK)");
        opts.push_back("6.  Chuyen theo so dien thoai");
        opts.push_back("7.  Yeu cau nhan tien (doi tien)");
        opts.push_back("8.  Xu ly yeu cau tien");
        opts.push_back("9.  Thanh toan hoa don");
        opts.push_back("10. Nap dien thoai");
        opts.push_back("11. Hu tiet kiem");
        opts.push_back("12. Xem sao ke / CSV");
        opts.push_back("13. Thong bao nhan tien");
        opts.push_back("14. Dat / doi PIN");
        opts.push_back("15. Doi han muc / ngay");
        opts.push_back("16. Trung tam tro giup");
        opts.push_back("17. Dang xuat");
        opts.push_back("0.  Thoat chuong trinh");
        int sel = UI::selectMenu("MENU KHACH HANG", opts, 0);

        if (sel == 0) hienThiTaiKhoan();
        else if (sel == 1) capNhatThongTin();
        else if (sel == 2) napTien();
        else if (sel == 3) rutTien();
        else if (sel == 4) chuyenKhoan();
        else if (sel == 5) chuyenTheoSdt();
        else if (sel == 6) taoYeuCauTien();
        else if (sel == 7) xuLyYeuCauTien();
        else if (sel == 8) thanhToanHoaDon();
        else if (sel == 9) napDienThoai();
        else if (sel == 10) quanLyTietKiem();
        else if (sel == 11) xemSaoKe();
        else if (sel == 12)
        {
            inTieuDe("THONG BAO NHAN TIEN");
            vector<string> tb = layThongBaoNhanTien(currentUser, 20);
            if (tb.empty()) UI::info("Chua nhan duoc khoan tien chuyen khoan nao.");
            else UI::notify("LICH SU NHAN TIEN", tb);
        }
        else if (sel == 13) datDoiPin();
        else if (sel == 14) doiHanMuc();
        else if (sel == 15) trungTamTroGiup();
        else if (sel == 16)
        {
            dangXuat();
            return;
        }
        else
        {
            dangXuat();
            if (db.isOpen())
                cout << "Du lieu da luu trong " << db.path() << ".\n";
            cout << "Tam biet!\n";
            exit(0);
        }
        if (vaiTro == VaiTro::KHACH)
            tamDung();
    }
}

void BankSystem::menuAdmin()
{
    while (vaiTro == VaiTro::ADMIN)
    {
        hienMenuAdmin();
        vector<string> opts;
        opts.push_back("1.  Mo tai khoan");
        opts.push_back("2.  Dong tai khoan");
        opts.push_back("3.  Tim kiem tai khoan");
        opts.push_back("4.  Hien thi thong tin TK");
        opts.push_back("5.  Cap nhat thong tin TK");
        opts.push_back("6.  Mo lai TK da dong");
        opts.push_back("7.  Nap tien (thu ngan)");
        opts.push_back("8.  Xem sao ke bat ky TK");
        opts.push_back("9.  Tra cuu giao dich theo ma");
        opts.push_back("10. Dao / hoan giao dich");
        opts.push_back("11. Danh sach tat ca TK");
        opts.push_back("12. Thong ke he thong");
        opts.push_back("13. Top so du cao nhat");
        opts.push_back("14. Tinh lai hang thang");
        opts.push_back("15. Mo khoa / dat lai PIN");
        opts.push_back("16. Doi han muc / ngay");
        opts.push_back("17. Nhat ky he thong");
        opts.push_back("18. Sao luu CSDL");
        opts.push_back("19. Quan ly du lieu");
        opts.push_back("20. Kiem tra Hash Table");
        opts.push_back("21. Dang xuat");
        opts.push_back("0.  Thoat chuong trinh");
        int sel = UI::selectMenu("MENU QUAN TRI (ADMIN)", opts, 0);

        if (sel == 0) moTaiKhoan();
        else if (sel == 1) dongTaiKhoan();
        else if (sel == 2) timTaiKhoan();
        else if (sel == 3) hienThiTaiKhoan();
        else if (sel == 4) capNhatThongTin();
        else if (sel == 5) moLaiTaiKhoan();
        else if (sel == 6) napTien();
        else if (sel == 7) xemSaoKe();
        else if (sel == 8) traCuuGiaoDich();
        else if (sel == 9) daoGiaoDich();
        else if (sel == 10) hienThiTatCa();
        else if (sel == 11) thongKe();
        else if (sel == 12) topTaiKhoan();
        else if (sel == 13) tinhLai();
        else if (sel == 14) moKhoaPin();
        else if (sel == 15) doiHanMuc();
        else if (sel == 16) xemNhatKy();
        else if (sel == 17) saoLuu();
        else if (sel == 18) quanLyDuLieu();
        else if (sel == 19) kiemTraHashTable();
        else if (sel == 20)
        {
            dangXuat();
            return;
        }
        else
        {
            dangXuat();
            if (db.isOpen())
                cout << "Du lieu da luu trong " << db.path() << ".\n";
            cout << "Tam biet!\n";
            exit(0);
        }
        if (vaiTro == VaiTro::ADMIN)
            tamDung();
    }
}

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
