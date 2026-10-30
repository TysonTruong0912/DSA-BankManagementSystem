# Báo cáo kiểm tra & sửa lỗi — Nexus Bank

## 1. Bản sao lưu
- `/home/workdir/artifacts/NexusBank_BACKUP_truoc_sua/` — mã nguồn trước khi sửa.

## 2. Lỗi đã sửa (có căn cứ)

| # | Vấn đề | File / hàm | Nguyên nhân | Cách sửa |
|---|--------|------------|-------------|----------|
| 1 | Hiện mật khẩu Admin + PIN mẫu trên màn hình chào | `BankSystem::hienManHinhChao` | In trực tiếp `admin123`, `PIN: 123456` | Bỏ hiển thị credential; chỉ liệt kê STK/tên/số dư + ghi chú xem README |
| 2 | In PIN khi tạo dữ liệu mẫu | `taoDuLieuMau`, `khoiTao` | Log demo lộ PIN | Đổi thông báo, hướng về README |
| 3 | Yêu cầu nhận tiền chỉ trong RAM | `dsYeuCau`, `taoYeuCauTien`, `xuLyYeuCauTien` | `vector` không lưu file/DB | Thêm bảng `money_requests`, `save/load/update` trong `Database`, nạp lúc `khoiTao` |
| 4 | Thiếu bộ test / README sai số test | `tests/`, README | Không có `tests/run_tests.py` | Tạo 14 test tự động kiểm tra thật |
| 5 | ZIP lồng 2 bản source | cấu trúc gói | Root + `NexusBank_Full/` trùng | Gói sạch 1 thư mục nguồn |

## 3. Biên dịch & kiểm thử (đã chạy trên Linux sandbox)

| Hạng mục | Kết quả |
|----------|---------|
| `make SQLITE=amalgam` | Thành công |
| `make SQLITE=none` | Thành công |
| `python3 tests/run_tests.py` | **14 PASS / 0 FAIL** |
| Windows thật | **Chưa** chạy trên máy Windows trong phiên này |

### Chi tiết test đã PASS
1. Khởi động / tạo dữ liệu mẫu  
2. Không in `admin123` trên màn hình chào  
3. Không in `PIN: 123456` trên màn hình chào  
4. Có số TK mẫu  
5. Sai PIN 3 lần → khóa  
6. Admin đăng nhập  
7. Admin mở khóa PIN  
8. Đăng nhập khách sau mở khóa  
9. Chuyển khoản thành công  
10. Tạo `bank.db`  
11. Khởi động lại đọc CSDL  
12. PIN lưu dạng hash  
13. Hash ≠ PIN gốc  
14. Bảng `money_requests` tồn tại  

## 4. Hạn chế / chưa xác minh
- SHA-256 lặp 2000 vòng **không** ngang Argon2/bcrypt — đủ cho đồ án, không đủ production.
- Admin vẫn hardcode trong code (demo); production cần hash + cấu hình riêng.
- PIN nhập vẫn hiện trên console (hạn chế terminal).
- Mutex trên HashTable **không** làm toàn app an toàn đa luồng (con trỏ `search`/`getAllAccounts`).
- Giao diện Windows Terminal: đã thiết kế ANSI; **chưa** xác nhận trên Windows thật trong phiên này.
- Đảo giao dịch / đối soát: không phát hiện lỗi logic khi đọc code; chưa có test tự động riêng cho mọi nhánh.

## 5. Thông tin đăng nhập kiểm thử (README)
- Admin: `admin` / `admin123`
- Khách mẫu: PIN `123456`

## 6. Gói bàn giao
- `NexusBank_FIXED.zip` — mã sạch + tests + README
- `NexusBank_BACKUP_truoc_sua/` — bản trước sửa
