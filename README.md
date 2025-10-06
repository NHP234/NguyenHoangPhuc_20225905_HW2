# DNS Resolver Application

Ứng dụng tra cứu DNS được viết bằng C, hỗ trợ cả phân giải thuận (domain → IP) và phân giải nghịch (IP → domain).

## Yêu cầu hệ thống
- Ubuntu 20.04 hoặc các hệ điều hành Linux tương tự
- GCC compiler
- Make

## Biên dịch

Để biên dịch chương trình, chạy lệnh:

```bash
make
```

Lệnh này sẽ tạo ra file thực thi `resolver`.

## Cách sử dụng

### Cú pháp
```bash
./resolver <domain_name|IP_address>
```

### Ví dụ

#### 1. Phân giải thuận (Domain → IP)
```bash
./resolver google.com
./resolver facebook.com
./resolver hust.edu.vn
```

**Kết quả mẫu:**
```
Domain name: google.com
IP addresses:
  142.250.185.206
  2404:6800:4003:c03::71
```

#### 2. Phân giải nghịch (IP → Domain)
```bash
./resolver 8.8.8.8
./resolver 1.1.1.1
```

**Kết quả mẫu:**
```
IP address: 8.8.8.8
Official hostname: dns.google
Additional domain names:
  ...
```

#### 3. Trường hợp không tìm thấy
```bash
./resolver invalid.domain.xyz
```

**Kết quả:**
```
Not found information
```

## Dọn dẹp

Để xóa file thực thi đã biên dịch:

```bash
make clean
```

## Tính năng

- ✅ Phân giải thuận: Chuyển đổi tên miền thành địa chỉ IP (hỗ trợ cả IPv4 và IPv6)
- ✅ Phân giải nghịch: Chuyển đổi địa chỉ IP thành tên miền
- ✅ Kiểm tra tính hợp lệ của địa chỉ IPv4
- ✅ Xử lý lỗi và hiển thị thông báo phù hợp
- ✅ Tương thích với Ubuntu 20.04

## Cấu trúc mã nguồn

- `resolver.c`: Mã nguồn chính của ứng dụng
- `Makefile`: File để biên dịch chương trình
- `README.md`: Tài liệu hướng dẫn

## Ghi chú

- Chương trình sử dụng các system calls chuẩn POSIX: `getaddrinfo()`, `getnameinfo()`, `inet_pton()`, `inet_ntop()`
- Hỗ trợ cả IPv4 và IPv6
- Kết quả tra cứu phụ thuộc vào cấu hình DNS của hệ thống

