import socket, sys, threading

# Tham số dòng lệnh: cổng, file xâu chào, file lưu nội dung client gửi
port, greet_file, save_file = int(sys.argv[1]), sys.argv[2], sys.argv[3]

# Đọc xâu chào từ file
greeting = open(greet_file).read()

def handle(conn, addr):
    print(f"[tcpserver] Client connected: {addr[0]}:{addr[1]}")

    # Gửi xâu chào
    conn.sendall(greeting.encode())

    # Ghi nội dung client gửi vào file
    data = b""
    while chunk := conn.recv(4096):
        data += chunk
    open(save_file, "ab").write(data)
    conn.close()
    print(f"[tcpserver] Client disconnected: {addr[0]}:{addr[1]}, saved to {save_file}")

srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
# Đợi kết nối ở cổng xác định bởi tham số dòng lệnh
srv.bind(("", port))
srv.listen()
print(f"[tcpserver] Listening on port {port}...")

# Mỗi khi có client kết nối đến, xử lý trong thread riêng
while True:
    conn, addr = srv.accept()
    threading.Thread(target=handle, args=(conn, addr)).start()