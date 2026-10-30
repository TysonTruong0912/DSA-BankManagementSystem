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

// === BankSystem_Reports.cpp ===

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


