#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("[Echo] 127.0.0.1 on port %d...\n", port);

    char buf[BUF_SIZE];

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);

        buf[n] = '\0';

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int  client_port = ntohs(client_addr.sin_port);

        printf("[Echo] From %s:%d To: \"%s\"\n", client_ip, client_port, buf);

        sendto(sock, buf, n, 0,
               (struct sockaddr *)&client_addr, client_len);
    }

    close(sock);
    return 0;
}
