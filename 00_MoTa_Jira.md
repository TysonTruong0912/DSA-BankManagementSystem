# Phần 5/5 — UI, biên dịch, kiểm thử, hạn chế & kết luận

---

## 1. Giao diện (UI)

- Khung Unicode (`╔═╗`, `┌─┐`), tông **cyan/teal**.
- Banner **NEXUS BANK**, thẻ thống kê / thẻ khách (họ tên, STK, số dư, loại TK).
- `UI::selectMenu` — chọn mục bằng số, không chỉ liệt kê tĩnh.
- Thông báo nhận tiền, cảnh báo yêu cầu đang chờ.
- Tiếng Việt có dấu; căn khung theo độ rộng UTF-8.
- Windows: bật Virtual Terminal + UTF-8.

---

## 2. Biên dịch & chạy

| Môi trường | Lệnh |
|------------|------|
| Windows MinGW | `.\build_windows.bat` → `.\bank.exe` |
| Linux/macOS | `make` → `./bank` |
| Không SQLite | `make SQLITE=none` |

Cần `-I.` khi biên dịch để tìm `sqlite3.h` local.  
PowerShell: bắt buộc `.\build_windows.bat`.

**Demo nhanh**

1. Đăng nhập admin → xem danh sách TK / thống kê.  
2. Đăng nhập khách (vd. 100001 / PIN 123456) → chuyển SĐT, tạo yêu cầu tiền.  
3. Hũ tiết kiệm, nạp ĐT mock.  
4. Admin đảo một giao dịch; xem nhật ký.

---

## 3. Kiểm thử gợi ý

1. Đăng nhập đúng/sai (khách & admin).  
2. Khách chuyển STK / SĐT — số dư 2 bên đúng.  
3. Yêu cầu tiền: chờ → đồng ý / từ chối.  
4. Sai PIN 3 lần → khóa; admin mở khóa.  
5. Vượt hạn mức ngày bị chặn.  
6. Rollback khi lỗi CSDL giữa chừng (RAM không lệch).  
7. Đảo GD admin — số dư hoàn lại hợp lệ.  
8. Đối soát toàn vẹn sau nhiều thao tác.

---

## 4. Hạn chế

- PIN / mật khẩu admin vẫn hiện khi gõ (console).  
- Admin cố định demo (`admin`/`admin123`), chưa quản lý nhiều nhân viên.  
- Hóa đơn / nạp ĐT là **mock**, không kết nối nhà cung cấp thật.  
- Ứng dụng học tập/demo, chưa đủ chuẩn ngân hàng thương mại.

---

## 5. Kết luận

Phiên bản cập nhật hoàn thiện Nexus Bank với:

- Hash Table tự cài + SQLite ACID.  
- **Hai vai trò** Khách / Admin.  
- Giao dịch đầy đủ + tính năng ví (SĐT, đòi tiền, hũ TK, đảo GD).  
- UI console có màu, menu tương tác.

**Hướng phát triển:** ẩn PIN khi nhập; nhiều tài khoản admin; API/GUI; xuất báo cáo PDF tự động.

---

## 6. Checklist Jira phần 5

- [x] UI + biên dịch + kịch bản test
- [x] Hạn chế & kết luận
- [ ] Đính kèm 2–3 screenshot (đăng nhập, menu khách, menu admin)


---

## Cập nhật gần nhất
- Không lộ credential trên UI chào.
- Yêu cầu nhận tiền lưu SQLite (`money_requests`).
- Gói full buildable: `NexusBank_FIXED.zip` (nếu nộp kèm).
