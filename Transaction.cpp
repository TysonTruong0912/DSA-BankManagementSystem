#include "Transaction.h"

using namespace std;

Transaction::Transaction()
    : maGiaoDich(""), loai(NAP_TIEN), soTien(0), soDuTruoc(0), soDuSau(0),
      taiKhoanLienQuan(""), noiDung(""), thoiGian(""), maGiaoDichGoc("")
{
}

Transaction::Transaction(const string& ma, TransactionType l, double tien,
                         double truoc, double sau, const string& lienQuan,
                         const string& nd, const string& tg, const string& maGoc)
    : maGiaoDich(ma), loai(l), soTien(tien), soDuTruoc(truoc), soDuSau(sau),
      taiKhoanLienQuan(lienQuan), noiDung(nd), thoiGian(tg), maGiaoDichGoc(maGoc)
{
}

string Transaction::getMaGiaoDich() const { return maGiaoDich; }
TransactionType Transaction::getLoai() const { return loai; }
double Transaction::getSoTien() const { return soTien; }
double Transaction::getSoDuTruoc() const { return soDuTruoc; }
double Transaction::getSoDuSau() const { return soDuSau; }
string Transaction::getTaiKhoanLienQuan() const { return taiKhoanLienQuan; }
string Transaction::getNoiDung() const { return noiDung; }
string Transaction::getThoiGian() const { return thoiGian; }
string Transaction::getMaGiaoDichGoc() const { return maGiaoDichGoc; }

string Transaction::typeToString(TransactionType t)
{
    switch (t)
    {
    case NAP_TIEN:           return "NAP_TIEN";
    case RUT_TIEN:           return "RUT_TIEN";
    case CHUYEN_KHOAN:       return "CHUYEN_KHOAN";
    case NHAN_CHUYEN_KHOAN:  return "NHAN_CHUYEN_KHOAN";
    case TRA_LAI:            return "TRA_LAI";
    case PHI_GIAO_DICH:      return "PHI_GIAO_DICH";
    case DAO_GIAO_DICH:      return "DAO_GIAO_DICH";
    case THANH_TOAN_HD:      return "THANH_TOAN_HD";
    case NAP_DT:             return "NAP_DT";
    case GUI_TIET_KIEM:      return "GUI_TIET_KIEM";
    case RUT_TIET_KIEM:      return "RUT_TIET_KIEM";
    }
    return "UNKNOWN";
}

bool Transaction::stringToType(const string& s, TransactionType& out)
{
    if (s == "NAP_TIEN")          { out = NAP_TIEN; return true; }
    if (s == "RUT_TIEN")          { out = RUT_TIEN; return true; }
    if (s == "CHUYEN_KHOAN")      { out = CHUYEN_KHOAN; return true; }
    if (s == "NHAN_CHUYEN_KHOAN") { out = NHAN_CHUYEN_KHOAN; return true; }
    if (s == "TRA_LAI")           { out = TRA_LAI; return true; }
    if (s == "PHI_GIAO_DICH")     { out = PHI_GIAO_DICH; return true; }
    if (s == "DAO_GIAO_DICH")     { out = DAO_GIAO_DICH; return true; }
    if (s == "THANH_TOAN_HD")     { out = THANH_TOAN_HD; return true; }
    if (s == "NAP_DT")            { out = NAP_DT; return true; }
    if (s == "GUI_TIET_KIEM")     { out = GUI_TIET_KIEM; return true; }
    if (s == "RUT_TIET_KIEM")     { out = RUT_TIET_KIEM; return true; }
    return false;
}
