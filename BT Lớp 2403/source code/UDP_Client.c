#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 3) {
        return 1;
    }

    char *server_ip = argv[1];
    int   port      = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons((uint16_t)port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    printf("Nhap:\n");

    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];

    while (fgets(send_buf, BUF_SIZE, stdin) != NULL) {

        int len = strlen(send_buf);
        if (send_buf[len - 1] == '\n') {
            send_buf[len - 1] = '\0';
            len--;
        }

        sendto(sock, send_buf, len, 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        socklen_t server_len = sizeof(server_addr);
        int n = recvfrom(sock, recv_buf, BUF_SIZE - 1, 0,
                         (struct sockaddr *)&server_addr, &server_len);

        recv_buf[n] = '\0';

        printf("[Echo] %s\n", recv_buf);
    }

    close(sock);
    return 0;
}
