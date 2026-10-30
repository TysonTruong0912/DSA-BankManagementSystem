#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include "Account.h"
#include "HashTable.h"
#include "Transaction.h"

// Lop boc SQLite. Hash Table van la bo nho dem (cache) tra cuu O(1);
// SQLite la NOI LUU TRU CHINH (ACID, an toan khi mat dien / crash giua chung).
//
// Neu chuong trinh duoc bien dich KHONG co SQLite (khong define USE_SQLITE) thi
// isOpen() luon false va BankSystem tu dong quay ve che do file .txt nhu cu.
//
// Tien luu trong DB dang so nguyen "xu" (x100) de khong bi sai so dau cham dong.
class Database
{
private:
    void* handle;                   // sqlite3* (an kieu de header khong phu thuoc sqlite3.h)
    std::string dbPath;
    std::string err;

    bool exec(const char* sql);

public:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    static bool available();        // chuong trinh co duoc bien dich kem SQLite khong
    bool open(const std::string& path);
    void close();
    bool isOpen() const;
    const std::string& lastError() const;
    const std::string& path() const;
    std::string version() const;

    // Giao dich CSDL (transaction). rollback() khong ghi de lastError.
    bool begin();
    bool commit();
    void rollback();

    // Ghi du lieu
    bool saveAccount(const Account& acc);                       // them moi hoac cap nhat (khong dong vao giao dich)
    bool updateBalance(const std::string& soTK, double soDu);
    bool deleteAccount(const std::string& soTK);                // xoa ca lich su giao dich (CASCADE)
    bool insertTransaction(const std::string& soTK, const Transaction& t);
    bool setMeta(const std::string& key, const std::string& value);
    bool getMeta(const std::string& key, std::string& value);
    bool audit(const std::string& hanhDong, const std::string& soTK, const std::string& chiTiet);

    // Doc / ghi hang loat (tu quan ly giao dich CSDL ben trong)
    bool loadAll(HashTable& table, int& lastTxId);
    bool saveAll(HashTable& table, int lastTxId);               // thay the toan bo tai khoan + giao dich

    // Tien ich
    bool printAudit(int limit);
    bool backupTo(const std::string& file);                     // VACUUM INTO -> file .db moi, nhat quan
    bool integrityCheck(std::string& result);
    long long count(const std::string& table);

    // Yeu cau nhan tien (luu ben vung)
    bool saveMoneyRequest(const std::string& ma, const std::string& soTkGui,
                          const std::string& soTkTra, double soTien,
                          const std::string& noiDung, const std::string& trangThai,
                          const std::string& thoiGian);
    bool updateMoneyRequestStatus(const std::string& ma, const std::string& trangThai);
    // load: callback style via vectors
    bool loadMoneyRequests(std::vector<std::string>& ma,
                           std::vector<std::string>& soTkGui,
                           std::vector<std::string>& soTkTra,
                           std::vector<double>& soTien,
                           std::vector<std::string>& noiDung,
                           std::vector<std::string>& trangThai,
                           std::vector<std::string>& thoiGian);
                  // "accounts" | "transactions" | "audit_log"
};

#endif
