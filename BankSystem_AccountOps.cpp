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

// === BankSystem_AccountOps.cpp ===

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


