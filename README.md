# UDP DNS Resolver - Client/Server Application

Ứng dụng phân giải tên miền sử dụng mô hình UDP Client-Server.

## Cấu trúc thư mục

```
.
├── UDP_Server/          # Server code
│   ├── server.c         # Main server file
│   ├── dns_lookup.c     # DNS lookup functions
│   ├── dns_lookup.h
│   ├── validation.c     # Input validation
│   ├── validation.h
│   ├── utils.c          # Utility functions
│   ├── utils.h
│   ├── resolver.h       # Common header
│   └── Makefile         # Server build file
│
└── UDP_Client/          # Client code
    ├── client.c         # Main client file
    └── Makefile         # Client build file
```

## Biên dịch

### Biên dịch Server
```bash
cd UDP_Server
make
```

### Biên dịch Client
```bash
cd UDP_Client
make
```

### Biên dịch cả hai (từ thư mục gốc)
```bash
make
```

## Sử dụng

### Chạy Server
```bash
cd UDP_Server
./server <PortNumber>
```

Ví dụ:
```bash
./server 5500
```

Server sẽ:
- Lắng nghe trên cổng được chỉ định
- Nhận và xử lý yêu cầu DNS lookup từ client
- Ghi log vào file `log_20225905.txt`
- Không tự động kết thúc trong mọi tình huống

### Chạy Client
```bash
cd UDP_Client
./client <ServerIP> <PortNumber>
```

Ví dụ:
```bash
./client 127.0.0.1 5500
```

Client sẽ:
- Kết nối đến server theo địa chỉ và cổng được chỉ định
- Cho phép người dùng nhập tên miền hoặc địa chỉ IP
- Hiển thị kết quả từ server
- Thoát khi người dùng nhập một xâu rỗng (chỉ nhấn Enter)

## Ví dụ sử dụng

### Terminal 1 (Server):
```bash
cd UDP_Server
./server 5500
```

Output:
```
Server is running on port 5500
Waiting for requests...
```

### Terminal 2 (Client):
```bash
cd UDP_Client
./client 127.0.0.1 5500
```

Output và tương tác:
```
Connected to server 127.0.0.1:5500
Enter domain name or IP address (press Enter to quit):
> google.com
142.250.185.46

> 8.8.8.8
dns.google

> 
Exiting...
```

## Format Log File

Server ghi log vào file `log_20225905.txt` với format:
```
[dd/mm/yyyy hh:mm:ss]$Yêu cầu nhận được$Kết quả truy vấn
```

Ví dụ:
```
[12/10/2025 20:30:15]$google.com$142.250.185.46
[12/10/2025 20:30:23]$8.8.8.8$dns.google
[12/10/2025 20:30:30]$invalid.domain$Not found information
```

## Dọn dẹp

### Xóa file biên dịch của Server
```bash
cd UDP_Server
make clean
```

### Xóa file biên dịch của Client
```bash
cd UDP_Client
make clean
```

### Xóa tất cả (từ thư mục gốc)
```bash
make clean
```

## Yêu cầu hệ thống

- GCC compiler
- Linux/Unix environment
- Standard C libraries

## Lưu ý

- Server sử dụng UDP protocol (connectionless)
- Client có timeout 5 giây cho mỗi request
- Server không tự động tắt, cần dùng Ctrl+C để dừng
- Log file sẽ được tạo tự động trong thư mục UDP_Server

