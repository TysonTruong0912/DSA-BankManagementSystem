#include "Database.h"
#include "Utils.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

using namespace std;

#ifdef USE_SQLITE
// ====================================================================
// BAN DAY DU - dung SQLite
// ====================================================================
#include <sqlite3.h>

namespace
{
    long long toCent(double v) { return (long long)std::llround(v * 100.0); }
    double fromCent(long long c) { return (double)c / 100.0; }

    sqlite3* H(void* p) { return static_cast<sqlite3*>(p); }

    // Bao boc prepared statement (RAII)
    class Stmt
    {
    public:
        Stmt(sqlite3* db, const char* sql) : st(nullptr), rc(sqlite3_prepare_v2(db, sql, -1, &st, nullptr)) {}
        ~Stmt() { if (st) sqlite3_finalize(st); }
        Stmt(const Stmt&) = delete;
        Stmt& operator=(const Stmt&) = delete;

        bool ok() const { return rc == SQLITE_OK; }
        void text(int i, const string& s) { sqlite3_bind_text(st, i, s.c_str(), (int)s.size(), SQLITE_TRANSIENT); }
        void i64(int i, long long v) { sqlite3_bind_int64(st, i, v); }
        int step() { return sqlite3_step(st); }
        string col(int i)
        {
            const unsigned char* p = sqlite3_column_text(st, i);
            return p ? string((const char*)p) : string();
        }
        long long coli(int i) { return sqlite3_column_int64(st, i); }

    private:
        sqlite3_stmt* st;
        int rc;
    };

    // "GD000012" -> 12
    int txNumber(const string& ma)
    {
        if (ma.size() < 3 || ma.substr(0, 2) != "GD") return 0;
        string d = ma.substr(2);
        if (!isAllDigits(d) || d.size() > 9) return 0;
        return atoi(d.c_str());
    }

    const char* SCHEMA =
        "CREATE TABLE IF NOT EXISTS accounts ("
        "  so_tai_khoan    TEXT PRIMARY KEY,"
        "  ho_ten          TEXT NOT NULL,"
        "  so_dien_thoai   TEXT NOT NULL,"
        "  email           TEXT NOT NULL,"
        "  so_du_xu        INTEGER NOT NULL CHECK (so_du_xu >= 0),"
        "  trang_thai      INTEGER NOT NULL CHECK (trang_thai IN (0,1)),"
        "  ngay_mo         TEXT NOT NULL,"
        "  pin_hash        TEXT NOT NULL DEFAULT '',"
        "  pin_salt        TEXT NOT NULL DEFAULT '',"
        "  so_lan_sai_pin  INTEGER NOT NULL DEFAULT 0,"
        "  han_muc_ngay_xu INTEGER NOT NULL DEFAULT 5000000000,"
        "  loai_tk         TEXT NOT NULL DEFAULT 'THANH_TOAN',"
        "  cmnd            TEXT NOT NULL DEFAULT '',"
        "  ngay_sinh       TEXT NOT NULL DEFAULT ''"
        ");"
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  ma_gd        TEXT PRIMARY KEY,"
        "  so_tai_khoan TEXT NOT NULL REFERENCES accounts(so_tai_khoan) ON DELETE CASCADE,"
        "  loai         TEXT NOT NULL,"
        "  so_tien_xu   INTEGER NOT NULL CHECK (so_tien_xu > 0),"
        "  so_du_truoc_xu INTEGER NOT NULL,"
        "  so_du_sau_xu   INTEGER NOT NULL,"
        "  tk_lien_quan TEXT NOT NULL DEFAULT '',"
        "  noi_dung     TEXT NOT NULL DEFAULT '',"
        "  thoi_gian    TEXT NOT NULL,"
        "  ma_gd_goc    TEXT NOT NULL DEFAULT ''"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_tx_account ON transactions(so_tai_khoan);"
        "CREATE TABLE IF NOT EXISTS meta ("
        "  key   TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS audit_log ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  thoi_gian    TEXT NOT NULL,"
        "  hanh_dong    TEXT NOT NULL,"
        "  so_tai_khoan TEXT NOT NULL DEFAULT '',"
        "  chi_tiet     TEXT NOT NULL DEFAULT ''"
        ");";
}

bool Database::loadAll(HashTable& table, int& lastTxId)
{
    table.clear();
    lastTxId = 0;

    {
        Stmt s(H(handle),
               "SELECT so_tai_khoan, ho_ten, so_dien_thoai, email, so_du_xu, trang_thai, ngay_mo,"
               " pin_hash, pin_salt, so_lan_sai_pin, han_muc_ngay_xu, loai_tk, cmnd, ngay_sinh"
               " FROM accounts ORDER BY so_tai_khoan;");
        if (!s.ok()) { err = sqlite3_errmsg(H(handle)); return false; }
        int rc;
        while ((rc = s.step()) == SQLITE_ROW)
        {
            LoaiTaiKhoan loai = THANH_TOAN;
            Account::stringToLoai(s.col(11), loai);
            Account a(s.col(0), s.col(1), s.col(2), s.col(3), fromCent(s.coli(4)),
                      s.coli(5) == 1, s.col(6), loai, s.col(12), s.col(13));
            a.setPin(s.col(8), s.col(7));
            a.setSoLanSaiPin((int)s.coli(9));
            a.setHanMucNgay(fromCent(s.coli(10)));
            table.insert(a);
        }
        if (rc != SQLITE_DONE) { err = sqlite3_errmsg(H(handle)); return false; }
    }

    int maxId = 0;
    {
        // rowid = thu tu ghi -> giao dich cua moi tai khoan nam dung thu tu thoi gian
        Stmt s(H(handle),
               "SELECT so_tai_khoan, ma_gd, loai, so_tien_xu, so_du_truoc_xu, so_du_sau_xu,"
               " tk_lien_quan, noi_dung, thoi_gian, ma_gd_goc FROM transactions ORDER BY rowid;");
        if (!s.ok()) { err = sqlite3_errmsg(H(handle)); return false; }
        int rc;
        while ((rc = s.step()) == SQLITE_ROW)
        {
            TransactionType type;
            if (!Transaction::stringToType(s.col(2), type)) continue;
            Account* acc = table.search(s.col(0));
            if (acc == nullptr) continue;
            acc->addTransaction(Transaction(s.col(1), type, fromCent(s.coli(3)), fromCent(s.coli(4)),
                                            fromCent(s.coli(5)), s.col(6), s.col(7), s.col(8), s.col(9)));
            int n = txNumber(s.col(1));
            if (n > maxId) maxId = n;
        }
        if (rc != SQLITE_DONE) { err = sqlite3_errmsg(H(handle)); return false; }
    }

    string v;
    if (getMeta("last_tx_id", v) && isAllDigits(v) && v.size() <= 9)
        maxId = max(maxId, atoi(v.c_str()));
    lastTxId = maxId;
    return true;
}

bool Database::saveAll(HashTable& table, int lastTxId)
{
    if (!begin()) return false;
    bool ok = exec("DELETE FROM transactions;") && exec("DELETE FROM accounts;");

    vector<Account*> all = table.getAllAccounts();
    for (size_t i = 0; ok && i < all.size(); ++i)
    {
        ok = saveAccount(*all[i]);
        for (TransactionNode* cur = all[i]->getTransactionHead(); ok && cur != nullptr; cur = cur->next)
            ok = insertTransaction(all[i]->getSoTaiKhoan(), cur->data);
    }
    if (ok) ok = setMeta("last_tx_id", to_string(lastTxId));
    if (ok) ok = audit("DONG_BO_TOAN_BO", "", to_string(all.size()) + " tai khoan");
    if (ok) ok = commit();
    if (!ok)
    {
        string e = err;
        rollback();
        err = e;
    }
    return ok;
}

bool Database::printAudit(int limit)
{
    Stmt s(H(handle),
           "SELECT id, thoi_gian, hanh_dong, so_tai_khoan, chi_tiet FROM audit_log ORDER BY id DESC LIMIT ?1;");
    if (!s.ok()) { err = sqlite3_errmsg(H(handle)); return false; }
    s.i64(1, limit);

    cout << left << setw(6) << "ID" << "| " << setw(20) << "Thoi gian" << "| "
         << setw(18) << "Hanh dong" << "| " << setw(12) << "Tai khoan" << "| Chi tiet\n";
    cout << string(100, '-') << "\n";
    int rows = 0, rc;
    while ((rc = s.step()) == SQLITE_ROW)
    {
        cout << left << setw(6) << s.coli(0) << "| " << setw(20) << s.col(1) << "| "
             << setw(18) << s.col(2) << "| " << setw(12) << (s.col(3).empty() ? "-" : s.col(3))
             << "| " << s.col(4) << "\n";
        ++rows;
    }
    if (rc != SQLITE_DONE) { err = sqlite3_errmsg(H(handle)); return false; }
    if (rows == 0) cout << "(nhat ky trong)\n";
    return true;
}

bool Database::backupTo(const string& file)
{
    Stmt s(H(handle), "VACUUM INTO ?1;");
    if (!s.ok()) { err = sqlite3_errmsg(H(handle)); return false; }
    s.text(1, file);
    if (s.step() != SQLITE_DONE) { err = sqlite3_errmsg(H(handle)); return false; }
    return true;
}

bool Database::integrityCheck(string& result)
{
    Stmt s(H(handle), "PRAGMA integrity_check;");
    if (!s.ok()) { err = sqlite3_errmsg(H(handle)); return false; }
    if (s.step() != SQLITE_ROW) { err = sqlite3_errmsg(H(handle)); return false; }
    result = s.col(0);
    return true;
}

long long Database::count(const string& table)
{
    if (table != "accounts" && table != "transactions" && table != "audit_log") return -1;
    Stmt s(H(handle), ("SELECT COUNT(*) FROM " + table + ";").c_str());
    if (!s.ok() || s.step() != SQLITE_ROW) return -1;
    return s.coli(0);
}


bool Database::loadMoneyRequests(vector<string>& ma, vector<string>& soTkGui,
                                 vector<string>& soTkTra, vector<double>& soTien,
                                 vector<string>& noiDung, vector<string>& trangThai,
                                 vector<string>& thoiGian)
{
    ma.clear(); soTkGui.clear(); soTkTra.clear(); soTien.clear();
    noiDung.clear(); trangThai.clear(); thoiGian.clear();
    Stmt s(H(handle),
           "SELECT ma, so_tk_gui, so_tk_tra, so_tien_xu, noi_dung, trang_thai, thoi_gian"
           " FROM money_requests ORDER BY rowid;");
    if (!s.ok()) { err = sqlite3_errmsg(H(handle)); return false; }
    int rc;
    while ((rc = s.step()) == SQLITE_ROW)
    {
        ma.push_back(s.col(0));
        soTkGui.push_back(s.col(1));
        soTkTra.push_back(s.col(2));
        soTien.push_back(fromCent(s.coli(3)));
        noiDung.push_back(s.col(4));
        trangThai.push_back(s.col(5));
        thoiGian.push_back(s.col(6));
    }
    if (rc != SQLITE_DONE) { err = sqlite3_errmsg(H(handle)); return false; }
    return true;
}

#else
// ====================================================================
// BAN RONG - bien dich khong co SQLite: moi thao tac that bai nhe nhang,
// BankSystem kiem tra isOpen() va chuyen sang che do file .txt
// ====================================================================

Database::Database() : handle(nullptr) {}
Database::~Database() {}
bool Database::available() { return false; }
bool Database::exec(const char*) { return false; }
bool Database::open(const string&) { err = "chuong trinh duoc bien dich khong kem SQLite"; return false; }
void Database::close() {}
bool Database::isOpen() const { return false; }
const string& Database::lastError() const { return err; }
const string& Database::path() const { return dbPath; }
string Database::version() const { return "(khong co)"; }
bool Database::begin() { return false; }
bool Database::commit() { return false; }
void Database::rollback() {}
bool Database::saveAccount(const Account&) { return false; }
bool Database::updateBalance(const string&, double) { return false; }
bool Database::deleteAccount(const string&) { return false; }
bool Database::insertTransaction(const string&, const Transaction&) { return false; }
bool Database::setMeta(const string&, const string&) { return false; }
bool Database::getMeta(const string&, string&) { return false; }
bool Database::audit(const string&, const string&, const string&) { return false; }
bool Database::loadAll(HashTable&, int&) { return false; }
bool Database::saveAll(HashTable&, int) { return false; }
bool Database::printAudit(int) { return false; }
bool Database::backupTo(const string&) { return false; }
bool Database::integrityCheck(string&) { return false; }
long long Database::count(const string&) { return -1; }
bool Database::saveMoneyRequest(const string&, const string&, const string&, double,
                                const string&, const string&, const string&) { return false; }
bool Database::updateMoneyRequestStatus(const string&, const string&) { return false; }
bool Database::loadMoneyRequests(vector<string>&, vector<string>&, vector<string>&,
                                 vector<double>&, vector<string>&, vector<string>&,
                                 vector<string>&) { return false; }


#endif

