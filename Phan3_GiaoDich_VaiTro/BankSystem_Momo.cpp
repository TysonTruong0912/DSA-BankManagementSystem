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

// === BankSystem_Momo.cpp ===

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


