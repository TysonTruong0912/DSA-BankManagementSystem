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

// === BankSystem_NapRutChuyen.cpp ===

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


