#ifndef BANKHELPERS_H
#define BANKHELPERS_H
#include <string>
#include <vector>
#include "Account.h"
#include "HashTable.h"
#include "Transaction.h"

namespace BankHelpers
{
    extern const std::string DB_FILE;
    extern const std::string ACCOUNT_FILE;
    extern const std::string TRANSACTION_FILE;
    extern const double MAX_AMOUNT;
    extern const double MAX_BALANCE;
    extern const int MAX_PIN_TRIES;
    extern const std::string PIN_MAU;
    extern const double PHI_RUT_TIEN;
    extern const double PHI_CHUYEN_KHOAN;
    extern const int SO_LAN_MIEN_PHI_CHUYEN;
    extern const double LAI_HU_TIET_KIEM;
    extern const std::string ADMIN_USER;
    extern const std::string ADMIN_PASS;

    std::string str(long long v);
    std::string maGiaoDich(int id);
    std::string docDong(const std::string& prompt);
    void inTieuDe(const std::string& title);
    void baoLoi(const std::string& msg);
    std::string lamSach(std::string s);
    bool hopLeSoTaiKhoan(const std::string& s);
    bool hopLeSoDienThoai(const std::string& s);
    bool hopLeHoTen(const std::string& s);
    bool hopLeEmail(const std::string& e);
    bool hopLeCmnd(const std::string& s);
    bool hopLeNgaySinh(const std::string& s);
    bool phanTichSoTien(const std::string& s, double& out, std::string& loi);
    bool laTienRa(TransactionType t);
    double tinhPhiChuyen(Account* a);
    void tangSoLanChuyen(Account* a);
    void inBangGiaoDich(const std::vector<const Transaction*>& ds);
    void inChiTietGiaoDich(const std::string& soTK, const std::string& chuTK, const Transaction& t);
    void inBangTaiKhoan(const std::vector<Account*>& ds);
    bool xuatCsvSaoKe(const std::string& file, const std::vector<const Transaction*>& ds);
    std::vector<const Transaction*> layGiaoDich(const Account& acc);
    std::vector<std::string> layThongBaoNhanTien(const Account* acc, int maxN = 5);
    std::string sinhSoTaiKhoanNgauNhien(const HashTable& accounts);
}
#endif
