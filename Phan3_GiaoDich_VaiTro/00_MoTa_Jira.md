# Phần 3/5 — Đăng nhập, menu theo vai trò & nghiệp vụ giao dịch

---

## 1. Đăng nhập

| Đối tượng | Thông tin đăng nhập |
|-----------|---------------------|
| Khách | Số tài khoản + PIN (tài khoản mẫu PIN `123456`) |
| Admin | User `admin` / mật khẩu `admin123` |

- Sai PIN khách: tăng bộ đếm, ≥ 3 → khóa PIN (lưu CSDL).
- Admin **không** được rút / chuyển tiền hộ khách.
- Đăng xuất quay về màn chọn vai trò.

---

## 2. Menu Khách hàng

1. Xem thông tin tài khoản  
2. Cập nhật thông tin  
3. Nạp tiền  
4. Rút tiền  
5. Chuyển khoản (theo STK)  
6. **Chuyển theo số điện thoại**  
7. **Yêu cầu nhận tiền (đòi tiền)**  
8. **Xử lý yêu cầu tiền** (đồng ý / từ chối)  
9. **Thanh toán hóa đơn** (mock)  
10. **Nạp điện thoại** (mock)  
11. **Hũ tiết kiệm** (gửi / rút hũ)  
12. Xem sao kê / CSV  
13. Thông báo nhận tiền  
14. Đặt / đổi PIN  
15. Đổi hạn mức / ngày  
16. Trung tâm trợ giúp  
17. Đăng xuất · 0. Thoát  

Màn khách hiển thị thẻ: chủ TK, STK, số dư, loại TK + thông báo tiền đến gần đây.

---

## 3. Menu Quản trị (Admin)

1–6. Mở / đóng / tìm / xem / cập nhật / mở lại TK  
7. Nạp tiền (thu ngân)  
8–9. Sao kê bất kỳ TK, tra cứu theo mã GD  
10. **Đảo / hoàn giao dịch**  
11–14. Danh sách TK, thống kê, top số dư, tính lãi  
15–16. Mở khóa PIN, đổi hạn mức  
17–20. Nhật ký, sao lưu, quản lý dữ liệu, Hash Table  
21. Đăng xuất · 0. Thoát  

---

## 4. Luồng giao dịch an toàn (`thucThi`)

1. Chuẩn bị các `Buoc` (số dư mới, loại GD…).  
2. Tạo sẵn `Transaction`.  
3. SQLite: `BEGIN` → cập nhật → `COMMIT`.  
4. Lỗi → `ROLLBACK`, RAM không đổi.  
5. Thành công → cập nhật Hash Table + linked list GD.

Áp dụng cho nạp, rút, chuyển STK/SĐT, yêu cầu tiền được chấp nhận, hóa đơn, nạp ĐT, hũ TK, đảo GD.

---

## 5. Tính năng kiểu ví (MoMo-like)

| Tính năng | Mô tả ngắn |
|-----------|------------|
| Chuyển theo SĐT | Tìm TK hoạt động theo SĐT người nhận → chuyển như STK |
| Yêu cầu nhận tiền | Người A tạo yêu cầu cho B; B đồng ý → trừ B cộng A |
| Hóa đơn / nạp ĐT | Mock nhà cung cấp, trừ tiền + ghi GD |
| Hũ tiết kiệm | `GUI_TIET_KIEM` / `RUT_TIET_KIEM` trên cùng một TK |
| Đảo GD (admin) | Hoàn theo mã GD gốc, ghi `DAO_GIAO_DICH` |

Rút / chuyển: kiểm tra PIN, số dư, **hạn mức ngày**.

---

## 6. Checklist Jira phần 3

- [x] Phân quyền Khách / Admin
- [x] Danh sách menu 2 phía
- [x] Tính năng chuyển SĐT, đòi tiền, hũ TK, đảo GD
- [ ] Screenshot menu khách + admin


---

## Cập nhật bảo mật hiển thị
- Màn hình chào **không** in PIN mẫu hay mật khẩu Admin.
- Thông tin demo (`admin`/`admin123`, PIN `123456`) chỉ ghi trong README / mô tả Jira để kiểm thử.
