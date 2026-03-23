import socket, sys

# Kết nối đến máy chủ xác định bởi địa chỉ IP và cổng
s = socket.socket()
s.connect((sys.argv[1], int(sys.argv[2])))
print(f"[tcpclient] Connected to {sys.argv[1]}:{sys.argv[2]}")

# Nhận dữ liệu từ server (xâu chào)
print(s.recv(4096).decode())

# Nhận dữ liệu từ bàn phím và gửi đến server
for line in sys.stdin:
    s.sendall(line.encode())
    print(f"[tcpclient] Sent: {line.strip()}")

s.close()
print("[tcpclient] Connection closed.")