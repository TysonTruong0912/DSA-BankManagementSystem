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
        ");"
        "CREATE TABLE IF NOT EXISTS money_requests ("
        "  ma           TEXT PRIMARY KEY,"
        "  so_tk_gui    TEXT NOT NULL,"
        "  so_tk_tra    TEXT NOT NULL,"
        "  so_tien_xu   INTEGER NOT NULL CHECK (so_tien_xu > 0),"
        "  noi_dung     TEXT NOT NULL DEFAULT '',"
        "  trang_thai   TEXT NOT NULL,"
        "  thoi_gian    TEXT NOT NULL"
        ");";
}

Database::Database() : handle(nullptr) {}
Database::~Database() { close(); }

bool Database::available() { return true; }

bool Database::exec(const char* sql)
{
    char* msg = nullptr;
    int rc = sqlite3_exec(H(handle), sql, nullptr, nullptr, &msg);
    if (rc != SQLITE_OK)
    {
        err = msg ? msg : "loi SQLite";
        sqlite3_free(msg);
        return false;
    }
    return true;
}

bool Database::open(const string& path)
{
    close();
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
        err = db ? sqlite3_errmsg(db) : "khong mo duoc file CSDL";
        if (db) sqlite3_close(db);
        return false;
    }
    handle = db;
    dbPath = path;
    sqlite3_busy_timeout(db, 3000);

    if (!exec("PRAGMA foreign_keys = ON;") || !exec("PRAGMA journal_mode=WAL;") || !exec(SCHEMA))
    {
        sqlite3_close(db);
        handle = nullptr;
        return false;
    }
    // Migration: them cot moi neu DB cu chua co (ALTER an toan, bo qua neu da co)
    exec("ALTER TABLE accounts ADD COLUMN loai_tk TEXT NOT NULL DEFAULT 'THANH_TOAN';");
    exec("ALTER TABLE accounts ADD COLUMN cmnd TEXT NOT NULL DEFAULT '';");
    exec("ALTER TABLE accounts ADD COLUMN ngay_sinh TEXT NOT NULL DEFAULT '';");
    exec("ALTER TABLE transactions ADD COLUMN ma_gd_goc TEXT NOT NULL DEFAULT '';");
    return true;
}

void Database::close()
{
    if (handle)
    {
        sqlite3_close(H(handle));
        handle = nullptr;
    }
}

bool Database::isOpen() const { return handle != nullptr; }
const string& Database::lastError() const { return err; }
const string& Database::path() const { return dbPath; }
string Database::version() const { return sqlite3_libversion(); }

bool Database::begin() { return exec("BEGIN IMMEDIATE;"); }
bool Database::commit() { return exec("COMMIT;"); }
void Database::rollback() { sqlite3_exec(H(handle), "ROLLBACK;", nullptr, nullptr, nullptr); }

#endif
