# He thong quan ly tai khoan & giao dich ngan hang (C++11 / Hash Table tu cai dat / SQLite)

Chuong trinh console C++11. Cau truc du lieu chinh van la **Hash Table tu cai dat**
(`Node** table` + Separate Chaining + Rehash, **khong** dung `unordered_map`).
Phien ban nay them **co so du lieu SQLite**, **PIN**, **han muc ngay**, **tinh lai**, **nhat ky he thong**... (xem ben duoi).

## Bien dich & chay

| Moi truong | Lenh |
|---|---|
| **Windows** (MinGW-w64 / MSYS2 / w64devkit) | `build_windows.bat` -> `bank.exe` |
| **Linux / macOS / MSYS2** | `make` -> `./bank` |
| Chi muon chay thu | Chay thang `bank.exe` (da bien dich san, xem luu y o muc cuoi) |

`sqlite3.c` / `sqlite3.h` (SQLite 3.45.1, public domain) da kem san trong thu muc -> **khong can cai them gi**.
Makefile tu chon: `sqlite3.c` trong thu muc -> SQLite cai san cua he thong -> khong SQLite.
Ep che do: `make SQLITE=system|amalgam|none`. Voi `SQLITE=none` chuong trinh chay **y nhu ban cu** (luu bang file .txt).

Kiem thu tu dong: `make && python3 tests/run_tests.py` (16 kiem tra).

## Luu tru du lieu

```
         +------------------+      ghi truoc (BEGIN..COMMIT)      +----------------+
 menu -> |  BankSystem      | ----------------------------------> |  SQLite        |
         |  Hash Table (RAM)| <-- doc 1 lan luc khoi dong ------- |  bank.db       |
         +------------------+   chi cap nhat RAM khi COMMIT OK    +----------------+
```

* **SQLite la noi luu chinh**, Hash Table la bo nho dem (cache) de tra cuu O(1). Moi giao dich duoc luu **ngay lap tuc** - khong con "nho luu truoc khi thoat", mat dien giua chung khong mat du lieu.
* **Nguyen tu (ACID)**: chuyen khoan ghi ca 2 ben + ma giao dich + nhat ky trong **mot** transaction CSDL. Loi o bat ky buoc nao -> `ROLLBACK`, RAM khong doi -> khong the "tru A ma chua cong B". (Da kiem thu bang cach cai trigger pha hoai vao CSDL.)
* Tien luu dang **so nguyen "xu"** (x100) -> khong sai so dau cham dong. Rang buoc `CHECK(so_du >= 0)`, khoa ngoai `ON DELETE CASCADE`.
* **Lan chay dau**: neu thay `accounts.txt` / `transactions.txt` -> tu **di tru** vao `bank.db` (dung de giu du lieu cu); khong co -> tao 5 tai khoan mau.
* File `.txt` van dung duoc de xuat/nhap/sao luu (menu 21). Dinh dang cu 7 cot van doc duoc.
* Khong co SQLite hoac loi mo CSDL -> tu dong quay ve che do file .txt.

Bang: `accounts`, `transactions`, `meta`, `audit_log`.

## Chuc nang (menu)

| Nhom | Muc | Mo ta |
|---|---|---|
| Tai khoan | 1 Mo TK | Kiem tra hop le + **dat PIN** + nap dau (1 giao dich CSDL) |
| | 2 Dong TK | Khoa (giu lich su) hoac xoa vinh vien (chi khi so du = 0) |
| | 3 Tim kiem | **MOI**: theo so TK (Hash O(1)), **ten / SDT / email** (chua tu khoa, khong phan biet hoa thuong) |
| | 4 Hien thi | Them thong tin PIN, han muc, da chi hom nay |
| | 5 Cap nhat thong tin | **MOI**: doi ten / SDT / email (can PIN) |
| | 6 Mo lai TK | **MOI**: mo lai tai khoan da dong |
| Giao dich | 7 Nap | |
| | 8 Rut | **MOI**: can PIN + kiem tra han muc ngay |
| | 9 Chuyen khoan | **MOI**: can PIN + han muc; nguyen tu |
| | 10 Sao ke | **MOI**: loc N gan nhat / khoang ngay / loai GD; tong vao-ra; **xuat CSV** |
| | 11 Tra cuu theo ma GD | **MOI**: hien ca ban ghi doi ung cua chuyen khoan |
| Bao cao | 12 Tat ca TK | |
| | 13 Thong ke | **MOI**: tien theo loai, GD hom nay, so TK co PIN/khoa, thong tin CSDL |
| | 14 Top so du | **MOI** |
| | 15 Tinh lai thang | **MOI**: tra lai %/thang cho moi TK hoat dong, 1 giao dich CSDL, canh bao tinh lai trung thang |
| Bao mat | 16 Dat/doi PIN | **MOI** |
| | 17 Mo khoa / dat lai PIN | **MOI**: nhan vien mo khoa sau khi sai 3 lan |
| | 18 Doi han muc/ngay | **MOI**: mac dinh 50,000,000 VND (rut + chuyen) |
| He thong | 19 Nhat ky | **MOI**: ai lam gi, luc nao (audit log trong CSDL) |
| | 20 Sao luu CSDL | **MOI**: `backup_yyyymmdd_hhmmss.db` (VACUUM INTO, nhat quan) |
| | 21 Quan ly du lieu | Xuat/nhap .txt, xuat CSV, du lieu mau, don dep TK test, **doi soat toan ven** |
| | 22 Kiem tra Hash Table | Demo rehash nay chay tren **bang tam** -> khong lam ban du lieu that |

### Bao mat PIN
* PIN 4-6 chu so, luu **salt + SHA-256 (2000 vong)**, khong luu PIN goc. Sai 3 lan -> khoa; so lan sai **luu trong CSDL** nen khoi dong lai van khoa.
* Tai khoan cu (chua co PIN) se duoc yeu cau dat PIN o lan rut/chuyen dau tien. Tai khoan mau co PIN `123456`.

### Doi soat (menu 21 -> 6)
Kiem tra: so du truoc/sau cua tung giao dich khop so tien; chuoi giao dich noi tiep nhau; so du hien tai = so du theo lich su; RAM khop CSDL; `PRAGMA integrity_check`.

## Hash Table
Khong doi so voi ban goc: hash `h = h*31 + c`, `index = h % capacity`; load factor > 0.75 -> rehash (capacity la so nguyen to nho nhat > 2x cu); insert/search/remove O(1) trung binh. `Account` du Rule of Three (copy sau); khong ro ri bo nho (da kiem tra AddressSanitizer + UBSan).

## Dinh dang file .txt
```
accounts.txt:     soTK|hoTen|sdt|email|soDu|trangThai|ngayMo|pinSalt|pinHash|soLanSaiPin|hanMuc   (hoac 7 cot dau - ban cu)
transactions.txt: #LAST_ID|n
                  soTK|maGD|loai|soTien|soDuTruoc|soDuSau|tkLienQuan|noiDung|thoiGian
loai: NAP_TIEN | RUT_TIEN | CHUYEN_KHOAN | NHAN_CHUYEN_KHOAN | TRA_LAI
```

## Goi y demo
1. Chay lan dau: xem dong "Da DI TRU du lieu tu file .txt vao CSDL". Menu 21 -> 5 de xoa 100 TK "Test User" cu.
2. Menu 9: chuyen khoan (lan dau se yeu cau dat PIN) -> menu 11 xem 2 ban ghi doi ung -> menu 19 xem nhat ky.
3. Tat chuong trinh bang Ctrl+C **ngay sau** giao dich, chay lai: du lieu van con (khac ban cu phai luu tay).
4. Menu 8 nhap sai PIN 3 lan -> bi khoa -> khoi dong lai van khoa -> menu 17 mo khoa.
5. Menu 15: tinh lai; chay lai de thay canh bao trung thang. Menu 21 -> 6 doi soat.
6. Menu 22 -> 4 nhap 60: xem `[REHASH] 5 -> 11 -> 23 -> 47 -> 97`.

## Luu y / han che (nen biet)
* Day la bai tap/demo: **nhap PIN hien tren man hinh** (console khong an ky tu), va **khong co dang nhap nhan vien** (ai chay chuong trinh cung dung duoc menu 17/20/21).
* `bank.exe` kem theo duoc **cross-compile tren Linux** (MinGW-w64, link tinh), chua duoc chay thu tren may Windows that. Neu loi, chay `build_windows.bat` de tu bien dich.
* Chuyen tu CSDL nay sang may khac: chi can copy `bank.db` canh `bank.exe`.
