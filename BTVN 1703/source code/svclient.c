#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int send_all(int sock, void *buf, int len) {
    int total = 0;
    char *p = (char *)buf;
    while (total < len) {
        int sent = send(sock, p + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return 0;
}

int send_uint8(int sock, uint8_t val) {
    return send_all(sock, &val, 1);
}

int send_uint32(int sock, uint32_t val) {
    uint32_t net_val = htonl(val);
    return send_all(sock, &net_val, 4);
}

int main(int argc, char *argv[]) {

    (void)argc;
    char *server_ip = argv[1];
    int   port      = atoi(argv[2]);

    uint32_t mssv = 0;
    char     hoten[128];
    char     ngaysinh[16];
    float    diem = 0.0f;

    printf("MSSV: "); scanf("%u",   &mssv);
    printf("Ho ten: "); scanf(" %127[^\n]", hoten);
    printf("Ngay sinh: "); scanf("%15s",  ngaysinh);
    printf("Diem TB: "); scanf("%f",    &diem);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons((uint16_t)port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    uint8_t  hoten_len    = (uint8_t)strlen(hoten);
    uint8_t  ngaysinh_len = (uint8_t)strlen(ngaysinh);
    uint32_t diem_int     = (uint32_t)(diem * 100 + 0.5f);  

    send_uint32(sock, mssv);
    send_uint8 (sock, hoten_len);
    send_all   (sock, hoten,    hoten_len);
    send_uint8 (sock, ngaysinh_len);
    send_all   (sock, ngaysinh, ngaysinh_len);
    send_uint32(sock, diem_int);
    close(sock);
    return 0;
}