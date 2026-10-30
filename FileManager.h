#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include "HashTable.h"

// Doc / ghi du lieu ra file text.
// Chi luu DU LIEU (khong luu con tro). Khi doc, tao lai Account va insert vao Hash Table.
//
// accounts.txt:      soTK|hoTen|soDienThoai|email|soDu|trangThai(1/0)|ngayMo|pinSalt|pinHash|soLanSaiPin|hanMuc
//                    (van doc duoc file cu chi co 7 cot dau)
// transactions.txt:  dong dau "#LAST_ID|n", cac dong sau:
//                    soTK|maGD|loai|soTien|soDuTruoc|soDuSau|tkLienQuan|noiDung|thoiGian
class FileManager
{
public:
    static bool saveAll(HashTable& table, int lastTxId,
                        const std::string& accountFile,
                        const std::string& transactionFile);

    // false neu khong mo duoc file tai khoan (khi do KHONG thay doi table)
    static bool loadAll(HashTable& table, int& lastTxId,
                        const std::string& accountFile,
                        const std::string& transactionFile);
};

#endif
