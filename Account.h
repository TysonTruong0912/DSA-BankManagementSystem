#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include "Transaction.h"

// Han muc rut + chuyen khoan toi da moi ngay (mac dinh khi mo tai khoan)
constexpr double MAC_DINH_HAN_MUC = 50000000.0;

// Loai tai khoan: anh huong lai suat va (co the) han muc
enum LoaiTaiKhoan
{
    THANH_TOAN = 0,     // tai khoan thanh toan, lai thap
    TIET_KIEM  = 1      // tai khoan tiet kiem, lai cao hon
};

// Lai suat mac dinh (%/thang)
constexpr double LAI_THANH_TOAN = 0.1;
constexpr double LAI_TIET_KIEM  = 0.5;

// Node cua danh sach lien ket don luu lich su giao dich
struct TransactionNode
{
    Transaction data;
    TransactionNode* next;

    TransactionNode(const Transaction& t) : data(t), next(nullptr) {}
};

class Account
{
private:
    std::string soTaiKhoan;
    std::string hoTen;
    std::string soDienThoai;
    std::string email;
    double soDu;
    bool trangThai;                 // true = dang hoat dong, false = da dong
    std::string ngayMo;

    // Lich su giao dich rieng cua tai khoan (linked list)
    TransactionNode* transactionHead;
    TransactionNode* transactionTail;   // giu them tail de them cuoi O(1)
    int transactionCount;

    // Bao mat: PIN luu dang BAM (salt + SHA-256), khong bao gio luu PIN goc
    std::string pinSalt;
    std::string pinHash;
    int soLanSaiPin;                // >= 3 -> PIN bi khoa
    double hanMucNgay;              // han muc rut + chuyen khoan / ngay

    // Hu tiet kiem noi bo (kieu Tui Than Tai thu nho)
    double soDuTietKiem;

    // Dem so lan chuyen khoan trong thang (phi mien phi N lan)
    int soLanChuyenThang;
    int thangChuyenKey;             // yyyymm

    // KYC + loai TK
    LoaiTaiKhoan loaiTaiKhoan;
    std::string cmnd;               // CMND/CCCD 9 hoac 12 so
    std::string ngaySinh;           // dd/mm/yyyy

    void clearTransactions();
    void copyTransactionsFrom(const Account& other);

public:
    Account();
    Account(const std::string& soTaiKhoan, const std::string& hoTen,
            const std::string& soDienThoai, const std::string& email,
            double soDu, bool trangThai, const std::string& ngayMo,
            LoaiTaiKhoan loai = THANH_TOAN,
            const std::string& cmnd = "",
            const std::string& ngaySinh = "");

    // Account tu quan ly bo nho nen can du "Rule of Three"
    Account(const Account& other);              // copy constructor (sao chep sau)
    Account& operator=(const Account& other);   // toan tu gan
    ~Account();                                 // giai phong linked list giao dich

    // Getter
    std::string getSoTaiKhoan() const;
    std::string getHoTen() const;
    std::string getSoDienThoai() const;
    std::string getEmail() const;
    double getSoDu() const;
    bool getTrangThai() const;
    std::string getNgayMo() const;
    int getTransactionCount() const;
    TransactionNode* getTransactionHead() const;
    std::string getPinSalt() const;
    std::string getPinHash() const;
    int getSoLanSaiPin() const;
    double getHanMucNgay() const;
    double getSoDuTietKiem() const;
    int getSoLanChuyenThang() const;
    int getThangChuyenKey() const;
    bool coPin() const;
    LoaiTaiKhoan getLoaiTaiKhoan() const;
    std::string getCmnd() const;
    std::string getNgaySinh() const;
    double getLaiSuatThang() const;             // %/thang theo loai TK

    // Setter
    void setHoTen(const std::string& v);
    void setSoDienThoai(const std::string& v);
    void setEmail(const std::string& v);
    void setSoDu(double v);
    void setTrangThai(bool v);
    void setPin(const std::string& salt, const std::string& hash);
    void xoaPin();
    void setSoLanSaiPin(int v);
    void setHanMucNgay(double v);
    void setSoDuTietKiem(double v);
    void setSoLanChuyenThang(int v);
    void setThangChuyenKey(int v);
    void setLoaiTaiKhoan(LoaiTaiKhoan v);
    void setCmnd(const std::string& v);
    void setNgaySinh(const std::string& v);

    // Them giao dich vao CUOI danh sach (de sao ke theo thu tu thoi gian)
    void addTransaction(const Transaction& t);

    // Hien thi
    void display() const;               // khung thong tin tai khoan
    void displayStatement() const;      // bang sao ke giao dich

    // Chuyen doi loai TK
    static std::string loaiToString(LoaiTaiKhoan l);
    static bool stringToLoai(const std::string& s, LoaiTaiKhoan& out);
};

#endif
