#include "BankHelpers.h"
#include "UI.h"
#include "Utils.h"
#include "BankSystem.h"
#include <algorithm>
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

namespace BankHelpers
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
    vector<string> layThongBaoNhanTien(const Account* acc, int maxN)
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

string sinhSoTaiKhoanNgauNhien(const HashTable& accounts)
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

} // namespace BankHelpers
