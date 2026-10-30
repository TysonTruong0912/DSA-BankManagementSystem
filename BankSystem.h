#ifndef BANKSYSTEM_H
#define BANKSYSTEM_H

#include <mutex>
#include <string>
#include <vector>
#include "Database.h"
#include "HashTable.h"
#include "Transaction.h"

// Nem ra khi dong nhap bi dong (Ctrl+D / het file) de thoat an toan,
// cac destructor van duoc goi -> khong ro ri bo nho.
struct InputClosed {};

// Vai tro phien dang nhap
enum class VaiTro
{
    KHONG,      // chua dang nhap
    KHACH,      // chu tai khoan (giao dich ca nhan)
    ADMIN       // quan tri (quan ly, khong rut/chuyen)
};

class BankSystem
{
private:
    HashTable accounts;     // Hash Table = bo nho dem tra cuu O(1) (key = so tai khoan)
    Database db;
    mutable std::mutex opsMutex;  // dong bo giao dich / ghi so du (multi-thread an toan)
    int lastTxId;           // so thu tu giao dich cuoi cung da cap
    std::string lastInterestMonth;  // "mm/yyyy" cua lan tinh lai gan nhat (chong tinh lai 2 lan)

    // Phien dang nhap
    VaiTro vaiTro;
    Account* currentUser;   // chi dung khi vaiTro == KHACH (khong so huu, chi tro toi node trong HashTable)

    // Yeu cau nhan tien (kieu MoMo "doi tien")
    struct YeuCauTien
    {
        std::string ma;
        std::string soTkGui;      // nguoi tao yeu cau (muon nhan)
        std::string soTkTra;      // nguoi bi yeu cau tra
        double soTien;
        std::string noiDung;
        std::string trangThai;    // CHO | DA_TRA | TU_CHOI
        std::string thoiGian;
    };
    std::vector<YeuCauTien> dsYeuCau;
    int lastYeuCauId;

    // Mot buoc thay doi so du + ghi 1 giao dich. Nhieu buoc duoc ghi trong CUNG mot giao dich CSDL.
    struct Buoc
    {
        Account* acc;
        double soDuMoi;
        TransactionType loai;
        double soTien;
        std::string lienQuan;
        std::string noiDung;
        std::string maGoc;      // ma GD goc (dao giao dich / phi)
    };

    // ---- Loi cua he thong: moi thay doi so du / giao dich deu di qua day ----
    bool thucThi(const std::vector<Buoc>& cacBuoc, const std::string& hanhDong,
                 const std::string& chiTiet, Account* taiKhoanMoi = nullptr,
                 const std::string& metaKey = "", const std::string& metaValue = "");
    bool luuTaiKhoan(const Account& acc, const std::string& hanhDong, const std::string& chiTiet);
    bool dongBoCSDL();

    // ---- Bao mat ----
    bool docPinMoi(std::string& pin);
    bool datPin(Account* acc, const std::string& hanhDong);
    bool xacThucPin(Account* acc);
    double tongDaChiHomNay(const Account* acc) const;
    bool kiemTraHanMuc(const Account* acc, double soTien) const;

    // ---- Dang nhap / phien ----
    bool dangNhapKhach();
    bool dangNhapAdmin();
    void dangXuat();

    // ---- Ham ho tro ----
    Transaction taoGiaoDichBoNho(TransactionType loai, double soTien, double soDuTruoc,
                                 double soDuSau, const std::string& lienQuan,
                                 const std::string& noiDung);
    Account* layTaiKhoan(const std::string& prompt);
    Account* layTaiKhoanHoatDong(const std::string& prompt);
    // Neu dang nhap KHACH: tra currentUser; Admin: hoi nhap so TK
    Account* layTaiKhoanTheoPhien(const std::string& promptHoatDong = "Nhap so tai khoan: ");
    bool docSoTien(const std::string& prompt, double& out, bool choPhepKhong);
    bool xacNhan(const std::string& cauHoi);
    void tamDung();
    void taoDuLieuMau();
    void taoTaiKhoanMau(const std::string& soTK, const std::string& hoTen, const std::string& sdt,
                        const std::string& email, double soDuBanDau);

    // ---- Menu theo vai tro ----
    void hienManHinhChao() const;
    void hienMenuKhach() const;
    void hienMenuAdmin() const;
    void menuKhach();
    void menuAdmin();

    // ---- Menu con ----
    void kiemTraHashTable();
    void demoRehash();
    void quanLyDuLieu();
    void doiSoat();
    void donDepTaiKhoanTest();

public:
    BankSystem();

    void khoiTao();

    // Quan ly tai khoan
    void moTaiKhoan();
    void dongTaiKhoan();
    void moLaiTaiKhoan();
    void timTaiKhoan();
    void hienThiTaiKhoan();
    void capNhatThongTin();

    // Giao dich
    void napTien();
    void rutTien();
    void chuyenKhoan();
    void xemSaoKe();
    void traCuuGiaoDich();
    void daoGiaoDich();
    void chuyenTheoSdt();           // chuyen tien theo so dien thoai
    void taoYeuCauTien();           // doi tien / yeu cau nhan tien
    void xuLyYeuCauTien();          // xem + dong y / tu choi
    void thanhToanHoaDon();         // hoa don mock
    void napDienThoai();            // nap DT mock
    void quanLyTietKiem();          // hu tiet kiem noi bo
    void trungTamTroGiup();         // help center

    // Bao cao
    void hienThiTatCa();
    void thongKe();
    void topTaiKhoan();
    void tinhLai();

    // Bao mat
    void datDoiPin();
    void moKhoaPin();
    void doiHanMuc();

    // He thong / file
    void xemNhatKy();
    void saoLuu();
    void luuDuLieu();
    void docDuLieu();

    void menu();            // man hinh chao -> dang nhap -> menu theo vai tro
};

#endif
