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

// === BankSystem_MenuKhach.cpp ===

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


