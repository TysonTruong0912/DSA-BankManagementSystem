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

// === BankSystem_MenuAdmin.cpp ===

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


