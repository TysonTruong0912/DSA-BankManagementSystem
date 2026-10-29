# Phần 1/5 — Tổng quan đề tài & Kiến trúc hệ thống

**Dự án:** Nexus Bank — Hệ thống quản lý tài khoản & giao dịch ngân hàng  
**Ngôn ngữ:** C++11 | **CSDL chính:** Hash Table tự cài | **Lưu trữ:** SQLite (+ file .txt)  
**Phiên bản:** cập nhật (đăng nhập theo vai trò, menu Khách / Admin, tính năng kiểu ví điện tử)

---

## 1. Mục tiêu

Xây dựng ứng dụng console quản lý ngân hàng với:

- **Hash Table tự cài** (Separate Chaining + Rehash), không dùng `std::unordered_map`.
- Lưu trữ **SQLite ACID**; Hash Table là cache RAM tra cứu O(1).
- **Phân quyền phiên đăng nhập:** Khách (chủ tài khoản) và Admin (quản trị).
- Giao dịch cơ bản + tính năng mở rộng kiểu ví: chuyển theo SĐT, yêu cầu nhận tiền, hóa đơn, nạp ĐT, hũ tiết kiệm.
- Bảo mật PIN, hạn mức ngày, đảo giao dịch (admin), nhật ký hệ thống.
- Giao diện console màu, khung Unicode, tiếng Việt có dấu, chọn menu bằng `UI::selectMenu`.

---

## 2. Luồng khởi động

```
main → UI::enableAnsi → BankSystem::khoiTao (load SQLite/txt)
     → menu() → màn hình chào
     → Đăng nhập Khách  HOẶC  Đăng nhập Admin
     → menuKhach()      HOẶC  menuAdmin()
```

| Vai trò | Cách vào | Quyền chính |
|---------|----------|-------------|
| **KHACH** | Số TK + PIN | Giao dịch cá nhân, không quản trị hệ thống |
| **ADMIN** | `admin` / `admin123` | Quản lý TK, nạp hộ, đảo GD, thống kê, CSDL — **không** rút/chuyển hộ |

---

## 3. Kiến trúc

```
              ┌──────────────────────────────┐
  UI/Menu ──► │         BankSystem           │
              │  vaiTro, currentUser, mutex  │
              │  dsYeuCau (yêu cầu nhận tiền)│
              └──────────────┬───────────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼                             ▼
     Hash Table (RAM)              Database SQLite
     key = số tài khoản            accounts, transactions
     Account + list GD             meta, audit_log
              │
              └── FileManager (.txt) xuất/nhập
```

**Đồng bộ:** ghi SQLite trước (`BEGIN`…`COMMIT`); chỉ khi thành công mới cập nhật RAM → chuyển khoản / đảo GD an toàn.

---

## 4. Module mã nguồn

| Module | Vai trò |
|--------|---------|
| `BankSystem` | Đăng nhập, menu 2 vai trò, toàn bộ nghiệp vụ |
| `HashTable` | Bảng băm, rehash, thống kê |
| `Account` | TK thanh toán / tiết kiệm, hũ TK, PIN, hạn mức |
| `Transaction` | Nhiều loại GD (kể cả phí, đảo, nạp ĐT, tiết kiệm) |
| `Database` | SQLite wrapper |
| `FileManager` | accounts.txt / transactions.txt |
| `Security` | SHA-256 + salt, hash PIN |
| `UI` | Banner, thẻ, selectMenu, thông báo |
| `Utils` | Tiền, ngày, chuỗi |

---

## 5. Checklist Jira phần 1

- [x] Mục tiêu & phạm vi phiên bản mới
- [x] Sơ đồ kiến trúc + phân quyền
- [ ] Đính kèm screenshot màn đăng nhập / chào
