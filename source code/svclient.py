import socket, sys, json

# Nhập dữ liệu thông tin sinh viên
mssv     = input("MSSV: ")
hoten    = input("Ho ten: ")
ngaysinh = input("Ngay sinh: ")
diem     = input("Diem TB: ")

# Đóng gói thông tin
data = json.dumps({"mssv": mssv, "hoten": hoten, "ngaysinh": ngaysinh, "diem": diem})

# Địa chỉ và cổng server lấy từ tham số dòng lệnh
s = socket.socket()
s.connect((sys.argv[1], int(sys.argv[2])))
print(f"[sv_client] Connected to {sys.argv[1]}:{sys.argv[2]}")

# Gửi sang sv_server
s.sendall(data.encode())
print(f"[sv_client] Sent: {mssv} {hoten} {ngaysinh} {diem}")
s.close()