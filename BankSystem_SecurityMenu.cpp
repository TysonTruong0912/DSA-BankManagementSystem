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

// === BankSystem_SecurityMenu.cpp ===

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


