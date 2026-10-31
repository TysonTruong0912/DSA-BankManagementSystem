#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

enum TransactionType
{
    NAP_TIEN,
    RUT_TIEN,
    CHUYEN_KHOAN,
    NHAN_CHUYEN_KHOAN,
    TRA_LAI,
    PHI_GIAO_DICH,
    DAO_GIAO_DICH,
    THANH_TOAN_HD,      // thanh toan hoa don mock
    NAP_DT,             // nap dien thoai mock
    GUI_TIET_KIEM,      // chuyen so du -> hu tiet kiem
    RUT_TIET_KIEM       // rut hu tiet kiem -> so du
};

class Transaction
{
private:
    std::string maGiaoDich;
    TransactionType loai;
    double soTien;
    double soDuTruoc;
    double soDuSau;
    std::string taiKhoanLienQuan;
    std::string noiDung;
    std::string thoiGian;
    std::string maGiaoDichGoc;

public:
    Transaction();
    Transaction(const std::string& maGiaoDich, TransactionType loai, double soTien,
                double soDuTruoc, double soDuSau, const std::string& taiKhoanLienQuan,
                const std::string& noiDung, const std::string& thoiGian,
                const std::string& maGiaoDichGoc = "");

    std::string getMaGiaoDich() const;
    TransactionType getLoai() const;
    double getSoTien() const;
    double getSoDuTruoc() const;
    double getSoDuSau() const;
    std::string getTaiKhoanLienQuan() const;
    std::string getNoiDung() const;
    std::string getThoiGian() const;
    std::string getMaGiaoDichGoc() const;

    static std::string typeToString(TransactionType t);
    static bool stringToType(const std::string& s, TransactionType& out);
};

#endif
