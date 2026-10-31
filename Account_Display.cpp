#include "Account.h"
#include "UI.h"
#include "Utils.h"

#include <cmath>
#include <iomanip>
#include <iostream>

using namespace std;

void Account::display() const
{
    UI::title("THÔNG TIN TÀI KHOẢN");
    cout << "  Số tài khoản : " << soTaiKhoan << "\n";
    cout << "  Họ tên       : " << hoTen << "\n";
    cout << "  CMND/CCCD    : " << (cmnd.empty() ? "(chưa cập nhật)" : cmnd) << "\n";
    cout << "  Ngày sinh    : " << (ngaySinh.empty() ? "(chưa cập nhật)" : ngaySinh) << "\n";
    cout << "  Số điện thoại: " << soDienThoai << "\n";
    cout << "  Email        : " << email << "\n";
    cout << "  Loại TK      : " << (loaiTaiKhoan == TIET_KIEM ? "Tiết kiệm" : "Thanh toán")
         << " (lãi " << getLaiSuatThang() << "%/tháng)\n";
    cout << "  Số dư        : " << formatMoney(soDu) << " VND\n";
    cout << "  Hũ tiết kiệm : " << formatMoney(soDuTietKiem) << " VND\n";
    cout << "  Trạng thái   : " << (trangThai ? "Đang hoạt động" : "Đã đóng") << "\n";
    cout << "  Ngày mở      : " << ngayMo << "\n";
    cout << "  Bảo mật      : " << (!coPin() ? "Chưa đặt PIN" : (soLanSaiPin >= 3 ? "PIN BỊ KHÓA" : "Đã đặt PIN")) << "\n";
    cout << "  Hạn mức/ngày : " << formatMoney(hanMucNgay) << " VND (rút + chuyển khoản)\n";
    UI::drawSeparator();
}

void Account::displayStatement() const
{
    UI::title("SAO KÊ TÀI KHOẢN");
    cout << "  Số tài khoản : " << soTaiKhoan << "\n";
    cout << "  Chủ tài khoản: " << hoTen << "\n";
    cout << "  Loại TK      : " << (loaiTaiKhoan == TIET_KIEM ? "Tiết kiệm" : "Thanh toán") << "\n\n";

    if (transactionHead == nullptr)
    {
        cout << "Tài khoản chưa có giao dịch.\n";
        return;
    }

    cout << left
         << setw(10) << "Ma GD" << "| "
         << setw(18) << "Loai GD" << "| "
         << setw(16) << "So tien" << "| "
         << setw(16) << "So du sau" << "| "
         << setw(20) << "Thoi gian" << "| "
         << setw(10) << "TK lien quan" << "| "
         << "Noi dung" << "\n";
    cout << string(115, '-') << "\n";

    for (TransactionNode* cur = transactionHead; cur != nullptr; cur = cur->next)
    {
        const Transaction& t = cur->data;

        // Tien ra (rut, chuyen khoan, phi, dao neu la tien ra) co dau -
        bool tienRa = (t.getLoai() == RUT_TIEN || t.getLoai() == CHUYEN_KHOAN ||
                       t.getLoai() == PHI_GIAO_DICH);
        // DAO_GIAO_DICH: dau phu thuoc noi dung (se xu ly o noiDung)
        if (t.getLoai() == DAO_GIAO_DICH)
        {
            // Dao cua NAP/NHAN -> tien ra; dao cua RUT/CHUYEN/PHI -> tien vao
            // Hien thi theo soDuSau vs soDuTruoc
            tienRa = (t.getSoDuSau() < t.getSoDuTruoc());
        }
        string tien = (tienRa ? "-" : "+") + formatMoney(t.getSoTien());

        cout << left
             << setw(10) << t.getMaGiaoDich() << "| "
             << setw(18) << Transaction::typeToString(t.getLoai()) << "| "
             << right << setw(16) << tien << left << "| "
             << right << setw(16) << formatMoney(t.getSoDuSau()) << left << "| "
             << setw(20) << t.getThoiGian() << "| "
             << setw(12) << (t.getTaiKhoanLienQuan().empty() ? "-" : t.getTaiKhoanLienQuan()) << "| "
             << t.getNoiDung();
        if (!t.getMaGiaoDichGoc().empty())
            cout << " [goc:" << t.getMaGiaoDichGoc() << "]";
        cout << "\n";
    }

    cout << string(115, '-') << "\n";
    cout << "Tổng số giao dịch: " << transactionCount
         << " | Số dư hiện tại: " << formatMoney(soDu) << " VND\n";
}
