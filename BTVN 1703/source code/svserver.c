#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int recv_all(int sock, void *buf, int len) {
    int total = 0;
    char *p = (char *)buf;
    while (total < len) {
        int received = recv(sock, p + total, len - total, 0);
        if (received <= 0) return -1;
        total += received;
    }
    return 0;
}

int recv_uint8(int sock, uint8_t *out) {
    return recv_all(sock, out, 1);
}

int recv_uint32(int sock, uint32_t *out) {
    uint32_t net_val = 0;
    int ret = recv_all(sock, &net_val, 4);
    if (ret < 0) return -1;
    *out = ntohl(net_val);
    return 0;
}

void handle_client(int client_sock, char *client_ip, FILE *flog) {

    uint32_t mssv      = 0;
    uint8_t  hoten_len = 0;
    char     hoten[128 + 1];
    uint8_t  ngaysinh_len = 0;
    char     ngaysinh[16 + 1];
    uint32_t diem_int  = 0;

    recv_uint32(client_sock, &mssv);
    recv_uint8 (client_sock, &hoten_len);
    recv_all   (client_sock, hoten, hoten_len);
    hoten[hoten_len] = '\0';
    recv_uint8 (client_sock, &ngaysinh_len);
    recv_all   (client_sock, ngaysinh, ngaysinh_len);
    ngaysinh[ngaysinh_len] = '\0';
    recv_uint32(client_sock, &diem_int);

    time_t    now = time(NULL);
    struct tm *t  = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    float diem = diem_int / 100.0f;

    printf("%s %s %u %s %s %.2f\n",
           timestamp, client_ip, mssv, hoten, ngaysinh, diem);
    fflush(stdout);

    fprintf(flog, "%s %s %u %s %s %.2f\n",
            client_ip, timestamp, mssv, hoten, ngaysinh, diem);
    fflush(flog);
}

int main(int argc, char *argv[]) {

    (void)argc;
    int   port     = atoi(argv[1]);
    char *log_path = argv[2];

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind  (server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("127.0.0.1 on port %d, log: %s\n", port, log_path);

    FILE *flog = fopen(log_path, "a");

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char client_ip[INET_ADDRSTRLEN];

        int client_sock = accept(server_sock,
                                 (struct sockaddr *)&client_addr,
                                 &client_len);

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        handle_client(client_sock, client_ip, flog);

        close(client_sock);
    }

    fclose(flog);
    close(server_sock);
    return 0;
}