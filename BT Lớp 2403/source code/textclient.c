#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 64

int main(int argc __attribute__((unused)), char *argv[]) {

    char   *server_ip = argv[1];
    int     port      = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons((uint16_t)port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Nhap:\n");

    char buf[BUF_SIZE];
    int  n;

    while ((n = read(STDIN_FILENO, buf, BUF_SIZE)) > 0) {
        send(sock, buf, n, 0);
    }

    close(sock);
    return 0;
}
