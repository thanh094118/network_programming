import socket, sys, json, threading
from datetime import datetime

# Tham số cổng và tên file log từ dòng lệnh
port, log_file = int(sys.argv[1]), sys.argv[2]

def handle(conn, addr):
    # Nhận dữ liệu từ sv_client
    data = json.loads(conn.recv(4096).decode())
    conn.close()

    # Lấy địa chỉ IP và thời gian client gửi
    t = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Dữ liệu ghi trên một dòng kèm địa chỉ IP và thời gian
    line = f"{addr[0]} {t} {data['mssv']} {data['hoten']} {data['ngaysinh']} {data['diem']}"

    # In ra màn hình
    print(f"[sv_server] {line}")

    # Đồng thời ghi vào file sv_log.txt
    open(log_file, "a", encoding="utf-8").write(line + "\n")
    print(f"[sv_server] Saved to {log_file}")

srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("", port))
srv.listen()
print(f"[sv_server] Listening on port {port}...")
while True:
    conn, addr = srv.accept()
    print(f"[sv_server] Client connected: {addr[0]}:{addr[1]}")
    threading.Thread(target=handle, args=(conn, addr)).start()