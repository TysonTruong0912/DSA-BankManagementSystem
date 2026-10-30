#include "FileManager.h"
#include "Utils.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

// Tach chuoi theo ky tu phan cach, GIU cac o rong (vi du "a||b" -> a, "", b)
static vector<string> split(const string& line, char delim)
{
    vector<string> parts;
    string current;
    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == delim)
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current += line[i];
        }
    }
    parts.push_back(current);
    return parts;
}

static bool parseDouble(const string& s, double& out)
{
    if (s.empty()) return false;
    char* end = nullptr;
    out = strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0' && std::isfinite(out);
}

// "GD000012" -> 12 ; sai dinh dang -> 0
static int parseTxNumber(const string& ma)
{
    if (ma.size() < 3 || ma.substr(0, 2) != "GD") return 0;
    string digits = ma.substr(2);
    if (!isAllDigits(digits) || digits.size() > 9) return 0;
    return atoi(digits.c_str());
}

bool FileManager::saveAll(HashTable& table, int lastTxId,
                          const string& accountFile, const string& transactionFile)
{
    ofstream accOut(accountFile.c_str());
    ofstream txOut(transactionFile.c_str());
    if (!accOut || !txOut)
        return false;

    accOut << fixed << setprecision(2);
    txOut << fixed << setprecision(2);

    txOut << "#LAST_ID|" << lastTxId << "\n";

    vector<Account*> all = table.getAllAccounts();
    for (size_t i = 0; i < all.size(); ++i)
    {
        const Account* a = all[i];
        accOut << a->getSoTaiKhoan() << "|"
               << a->getHoTen() << "|"
               << a->getSoDienThoai() << "|"
               << a->getEmail() << "|"
               << a->getSoDu() << "|"
               << (a->getTrangThai() ? 1 : 0) << "|"
               << a->getNgayMo() << "|"
               << a->getPinSalt() << "|"
               << a->getPinHash() << "|"
               << a->getSoLanSaiPin() << "|"
               << a->getHanMucNgay() << "|"
               << Account::loaiToString(a->getLoaiTaiKhoan()) << "|"
               << a->getCmnd() << "|"
               << a->getNgaySinh() << "\n";

        for (TransactionNode* cur = a->getTransactionHead(); cur != nullptr; cur = cur->next)
        {
            const Transaction& t = cur->data;
            txOut << a->getSoTaiKhoan() << "|"
                  << t.getMaGiaoDich() << "|"
                  << Transaction::typeToString(t.getLoai()) << "|"
                  << t.getSoTien() << "|"
                  << t.getSoDuTruoc() << "|"
                  << t.getSoDuSau() << "|"
                  << t.getTaiKhoanLienQuan() << "|"
                  << t.getNoiDung() << "|"
                  << t.getThoiGian() << "|"
                  << t.getMaGiaoDichGoc() << "\n";
        }
    }

    return accOut.good() && txOut.good();
}

bool FileManager::loadAll(HashTable& table, int& lastTxId,
                          const string& accountFile, const string& transactionFile)
{
    ifstream accIn(accountFile.c_str());
    if (!accIn)
        return false;

    // Chi xoa du lieu cu khi chac chan da mo duoc file
    table.clear();
    lastTxId = 0;

    string line;
    int lineNo = 0;

    // ---- Doc tai khoan ----
    while (getline(accIn, line))
    {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        vector<string> f = split(line, '|');
        double soDu;
        // 7 cot (cu) | 11 cot (+PIN) | 14 cot (+loai|cmnd|ngay_sinh)
        bool coBaoMat = (f.size() == 11 || f.size() == 14);
        bool coKyc = (f.size() == 14);
        if ((f.size() != 7 && !coBaoMat) || !parseDouble(f[4], soDu) || (f[5] != "0" && f[5] != "1"))
        {
            cout << "[CANH BAO] Bo qua dong loi " << lineNo << " trong " << accountFile << "\n";
            continue;
        }

        LoaiTaiKhoan loai = THANH_TOAN;
        string cmndVal, nsVal;
        if (coKyc)
        {
            Account::stringToLoai(f[11], loai);
            cmndVal = f[12];
            nsVal = f[13];
        }
        Account acc(f[0], f[1], f[2], f[3], soDu, f[5] == "1", f[6], loai, cmndVal, nsVal);
        if (coBaoMat)
        {
            double hanMuc;
            if (!isAllDigits(f[9]) || f[9].size() > 3 || !parseDouble(f[10], hanMuc) || hanMuc <= 0)
            {
                cout << "[CANH BAO] Bo qua dong loi " << lineNo << " trong " << accountFile << "\n";
                continue;
            }
            acc.setPin(f[7], f[8]);
            acc.setSoLanSaiPin(atoi(f[9].c_str()));
            acc.setHanMucNgay(hanMuc);
        }
        if (!table.insert(acc))     // insert tra false neu trung so tai khoan
            cout << "[CANH BAO] Trung so tai khoan " << f[0] << " (dong " << lineNo << "), bo qua.\n";
    }
    accIn.close();

    // ---- Doc giao dich (file nay khong bat buoc phai co) ----
    ifstream txIn(transactionFile.c_str());
    if (!txIn)
        return true;

    int maxId = 0;
    lineNo = 0;
    while (getline(txIn, line))
    {
        ++lineNo;
        line = trim(line);
        if (line.empty()) continue;

        vector<string> f = split(line, '|');

        if (line[0] == '#')                         // dong meta: #LAST_ID|n
        {
            if (f.size() == 2 && f[0] == "#LAST_ID" && isAllDigits(f[1]) && f[1].size() <= 9)
                maxId = max(maxId, atoi(f[1].c_str()));
            continue;
        }

        TransactionType type;
        double soTien, truoc, sau;
        if ((f.size() != 9 && f.size() != 10) || !Transaction::stringToType(f[2], type) ||
            !parseDouble(f[3], soTien) || !parseDouble(f[4], truoc) || !parseDouble(f[5], sau))
        {
            cout << "[CANH BAO] Bo qua dong loi " << lineNo << " trong " << transactionFile << "\n";
            continue;
        }

        Account* acc = table.search(f[0]);          // tim tai khoan bang Hash Table
        if (acc == nullptr)
        {
            cout << "[CANH BAO] Giao dich " << f[1] << " thuoc tai khoan khong ton tai, bo qua.\n";
            continue;
        }

        string maGoc = (f.size() >= 10) ? f[9] : "";
        acc->addTransaction(Transaction(f[1], type, soTien, truoc, sau, f[6], f[7], f[8], maGoc));
        maxId = max(maxId, parseTxNumber(f[1]));
    }

    lastTxId = maxId;
    return true;
}
