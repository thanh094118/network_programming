#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE  64
#define PAT_LEN   10

const char *PATTERN = "0123456789";

int count_pattern(char *tail, int *tail_len, char *buf, int len) {

    int    window_len = *tail_len + len;
    char  *window     = malloc(window_len + 1);

    memcpy(window, tail, *tail_len);
    memcpy(window + *tail_len, buf, len);
    window[window_len] = '\0';

    int   count = 0;
    char *pos   = window;

    while ((pos = strstr(pos, PATTERN)) != NULL) {
        count++;
        pos += PAT_LEN;
    }

    *tail_len = (window_len >= PAT_LEN - 1) ? PAT_LEN - 1 : window_len;
    memcpy(tail, window + window_len - *tail_len, *tail_len);

    free(window);
    return count;
}

void handle_client(int client_sock, char *client_ip) {

    printf("127.0.0.1 ket noi port 9000: \n");

    char buf[BUF_SIZE];
    int  n;
    int  total    = 0;

    char tail[PAT_LEN];
    int  tail_len = 0;
    memset(tail, 0, sizeof(tail));

    while ((n = recv(client_sock, buf, BUF_SIZE, 0)) > 0) {

        int clean_len = 0;
        for (int i = 0; i < n; i++) {
            if (buf[i] != '\n') {
                buf[clean_len++] = buf[i];
            }
        }

        int found = count_pattern(tail, &tail_len, buf, clean_len);
        total += found;
        printf("Nhan %d byte | Tim them: %d | Tong: %d\n", n, found, total);
        fflush(stdout);
    }

    printf("Tong so lan xuat hien: %d\n\n", total);
}

int main(int argc __attribute__((unused)), char *argv[]) {

    int port = atoi(argv[1]);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char client_ip[INET_ADDRSTRLEN];

        int client_sock = accept(server_sock,
                                 (struct sockaddr *)&client_addr,
                                 &client_len);

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        handle_client(client_sock, client_ip);
        close(client_sock);
    }

    close(server_sock);
    return 0;
}