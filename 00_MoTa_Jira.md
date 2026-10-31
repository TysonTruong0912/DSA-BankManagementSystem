# Phần 2/5 — Hash Table & mô hình Account / Transaction

---

## 1. Hash Table tự cài

| Thuộc tính | Chi tiết |
|------------|----------|
| Cấu trúc | `Node** table` + linked list mỗi bucket |
| Đụng độ | Separate Chaining |
| Hàm băm | `hash = hash * 31 + c` → `index % capacity` |
| Load factor | > **0.75** → rehash |
| Capacity | Số nguyên tố: 5 → 11 → 23 → 47 → 97… |
| Độ phức tạp | insert / search / remove ≈ **O(1)** trung bình |

**Rehash:** tạo bảng capacity mới, nối lại node cũ (không new Account) → con trỏ `Account*` vẫn hợp lệ.  
Menu Admin có mục **Kiểm tra Hash Table** + demo rehash trên bảng tạm.

Có thể dùng `std::mutex` ở tầng BankSystem để đồng bộ thao tác ghi số dư (an toàn hơn khi mở rộng đa luồng).

---

## 2. Account (mở rộng)

Ngoài thông tin cơ bản (STK, họ tên, SĐT, email, số dư, trạng thái, ngày mở):

| Trường / khái niệm | Ý nghĩa |
|--------------------|---------|
| `loaiTaiKhoan` | `THANH_TOAN` hoặc `TIET_KIEM` |
| `soDuTietKiem` | Số dư **hũ tiết kiệm** nội bộ |
| PIN | salt + hash, `soLanSaiPin`, khóa ≥ 3 lần |
| `hanMucNgay` | Hạn mức rút + chuyển / ngày (mặc định 50 triệu) |
| CMND / ngày sinh | Bổ sung hồ sơ (nếu có trong bản build) |
| Lãi suất | Phụ thuộc loại TK (thanh toán / tiết kiệm) |

**Rule of Three:** copy constructor, `operator=`, destructor — sao chép sâu linked list giao dịch.

---

## 3. Transaction — các loại

```text
NAP_TIEN | RUT_TIEN | CHUYEN_KHOAN | NHAN_CHUYEN_KHOAN | TRA_LAI
PHI_GIAO_DICH | DAO_GIAO_DICH | NAP_DT
GUI_TIET_KIEM | RUT_TIET_KIEM
```

- Chuyển khoản: 2 bản ghi (gửi + nhận).
- `DAO_GIAO_DICH`: hoàn theo mã GD gốc (admin).
- `GUI_TIET_KIEM` / `RUT_TIET_KIEM`: chuyển giữa số dư chính ↔ hũ tiết kiệm.
- `NAP_DT`: nạp điện thoại mock (trừ tiền, ghi GD).

Mỗi GD có: mã `GDxxxxxx`, số tiền, số dư trước/sau, TK liên quan, nội dung, thời gian; có thể gắn `maGoc` khi đảo/phí.

---

## 4. Checklist Jira phần 2

- [x] Hash Table + rehash
- [x] Account loại TK / hũ tiết kiệm
- [x] Đầy đủ enum Transaction
- [ ] Screenshot thống kê Hash Table (menu admin)
