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
                       t.getLoai() == PHI_GIAO_DICH || t.getLoai() == THANH_TOAN_HD ||
                       t.getLoai() == NAP_DT || t.getLoai() == GUI_TIET_KIEM);
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
