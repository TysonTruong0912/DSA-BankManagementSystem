#include "Account.h"
#include "UI.h"
#include "Utils.h"

#include <cmath>
#include <iomanip>
#include <iostream>

using namespace std;

Account::Account()
    : soTaiKhoan(""), hoTen(""), soDienThoai(""), email(""), soDu(0),
      trangThai(true), ngayMo(""), transactionHead(nullptr),
      transactionTail(nullptr), transactionCount(0),
      pinSalt(""), pinHash(""), soLanSaiPin(0), hanMucNgay(MAC_DINH_HAN_MUC),
      soDuTietKiem(0), soLanChuyenThang(0), thangChuyenKey(0),
      loaiTaiKhoan(THANH_TOAN), cmnd(""), ngaySinh("")
{
}

Account::Account(const string& stk, const string& ten, const string& sdt,
                 const string& mail, double du, bool tt, const string& ngay,
                 LoaiTaiKhoan loai, const string& cmndVal, const string& ns)
    : soTaiKhoan(stk), hoTen(ten), soDienThoai(sdt), email(mail), soDu(du),
      trangThai(tt), ngayMo(ngay), transactionHead(nullptr),
      transactionTail(nullptr), transactionCount(0),
      pinSalt(""), pinHash(""), soLanSaiPin(0), hanMucNgay(MAC_DINH_HAN_MUC),
      soDuTietKiem(0), soLanChuyenThang(0), thangChuyenKey(0),
      loaiTaiKhoan(loai), cmnd(cmndVal), ngaySinh(ns)
{
}

// Copy constructor: sao chep SAU (tao node moi cho tung giao dich)
// de hai Account khong dung chung con tro -> tranh double delete.
Account::Account(const Account& other)
    : soTaiKhoan(other.soTaiKhoan), hoTen(other.hoTen),
      soDienThoai(other.soDienThoai), email(other.email), soDu(other.soDu),
      trangThai(other.trangThai), ngayMo(other.ngayMo),
      transactionHead(nullptr), transactionTail(nullptr), transactionCount(0),
      pinSalt(other.pinSalt), pinHash(other.pinHash),
      soLanSaiPin(other.soLanSaiPin), hanMucNgay(other.hanMucNgay),
      soDuTietKiem(other.soDuTietKiem), soLanChuyenThang(other.soLanChuyenThang),
      thangChuyenKey(other.thangChuyenKey),
      loaiTaiKhoan(other.loaiTaiKhoan), cmnd(other.cmnd), ngaySinh(other.ngaySinh)
{
    copyTransactionsFrom(other);
}

Account& Account::operator=(const Account& other)
{
    if (this != &other)
    {
        clearTransactions();
        soTaiKhoan = other.soTaiKhoan;
        hoTen = other.hoTen;
        soDienThoai = other.soDienThoai;
        email = other.email;
        soDu = other.soDu;
        trangThai = other.trangThai;
        ngayMo = other.ngayMo;
        pinSalt = other.pinSalt;
        pinHash = other.pinHash;
        soLanSaiPin = other.soLanSaiPin;
        hanMucNgay = other.hanMucNgay;
        soDuTietKiem = other.soDuTietKiem;
        soLanChuyenThang = other.soLanChuyenThang;
        thangChuyenKey = other.thangChuyenKey;
        loaiTaiKhoan = other.loaiTaiKhoan;
        cmnd = other.cmnd;
        ngaySinh = other.ngaySinh;
        copyTransactionsFrom(other);
    }
    return *this;
}

Account::~Account()
{
    clearTransactions();
}

void Account::clearTransactions()
{
    TransactionNode* cur = transactionHead;
    while (cur != nullptr)
    {
        TransactionNode* next = cur->next;
        delete cur;
        cur = next;
    }
    transactionHead = nullptr;
    transactionTail = nullptr;
    transactionCount = 0;
}

void Account::copyTransactionsFrom(const Account& other)
{
    for (TransactionNode* cur = other.transactionHead; cur != nullptr; cur = cur->next)
        addTransaction(cur->data);
}

void Account::addTransaction(const Transaction& t)
{
    TransactionNode* node = new TransactionNode(t);
    if (transactionHead == nullptr)
    {
        transactionHead = transactionTail = node;
    }
    else
    {
        transactionTail->next = node;
        transactionTail = node;
    }
    ++transactionCount;
}

string Account::getSoTaiKhoan() const { return soTaiKhoan; }
string Account::getHoTen() const { return hoTen; }
string Account::getSoDienThoai() const { return soDienThoai; }
string Account::getEmail() const { return email; }
double Account::getSoDu() const { return soDu; }
bool Account::getTrangThai() const { return trangThai; }
string Account::getNgayMo() const { return ngayMo; }
int Account::getTransactionCount() const { return transactionCount; }
TransactionNode* Account::getTransactionHead() const { return transactionHead; }
string Account::getPinSalt() const { return pinSalt; }
string Account::getPinHash() const { return pinHash; }
int Account::getSoLanSaiPin() const { return soLanSaiPin; }
double Account::getHanMucNgay() const { return hanMucNgay; }
double Account::getSoDuTietKiem() const { return soDuTietKiem; }
int Account::getSoLanChuyenThang() const { return soLanChuyenThang; }
int Account::getThangChuyenKey() const { return thangChuyenKey; }
bool Account::coPin() const { return !pinHash.empty() && !pinSalt.empty(); }
LoaiTaiKhoan Account::getLoaiTaiKhoan() const { return loaiTaiKhoan; }
string Account::getCmnd() const { return cmnd; }
string Account::getNgaySinh() const { return ngaySinh; }

double Account::getLaiSuatThang() const
{
    return (loaiTaiKhoan == TIET_KIEM) ? LAI_TIET_KIEM : LAI_THANH_TOAN;
}

void Account::setHoTen(const string& v) { hoTen = v; }
void Account::setSoDienThoai(const string& v) { soDienThoai = v; }
void Account::setEmail(const string& v) { email = v; }
void Account::setTrangThai(bool v) { trangThai = v; }
void Account::setPin(const string& salt, const string& hash) { pinSalt = salt; pinHash = hash; }
void Account::xoaPin() { pinSalt.clear(); pinHash.clear(); soLanSaiPin = 0; }
void Account::setSoLanSaiPin(int v) { soLanSaiPin = v < 0 ? 0 : v; }
void Account::setHanMucNgay(double v) { hanMucNgay = std::round(v * 100.0) / 100.0; }
void Account::setSoDuTietKiem(double v) { soDuTietKiem = std::round(v * 100.0) / 100.0; }
void Account::setSoLanChuyenThang(int v) { soLanChuyenThang = v < 0 ? 0 : v; }
void Account::setThangChuyenKey(int v) { thangChuyenKey = v; }
void Account::setLoaiTaiKhoan(LoaiTaiKhoan v) { loaiTaiKhoan = v; }
void Account::setCmnd(const string& v) { cmnd = v; }
void Account::setNgaySinh(const string& v) { ngaySinh = v; }

void Account::setSoDu(double v)
{
    // Lam tron 2 chu so thap phan de tranh sai so dau cham dong
    soDu = std::round(v * 100.0) / 100.0;
}

string Account::loaiToString(LoaiTaiKhoan l)
{
    return (l == TIET_KIEM) ? "TIET_KIEM" : "THANH_TOAN";
}

bool Account::stringToLoai(const string& s, LoaiTaiKhoan& out)
{
    if (s == "TIET_KIEM" || s == "1") { out = TIET_KIEM; return true; }
    if (s == "THANH_TOAN" || s == "0") { out = THANH_TOAN; return true; }
    return false;
}

