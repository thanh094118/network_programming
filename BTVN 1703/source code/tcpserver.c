#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

    (void)argc;
    int   port         = atoi(argv[1]);
    char *hello_file   = argv[2];
    char *log_file     = argv[3];

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock,   (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char client_ip[INET_ADDRSTRLEN];

        int client_sock = accept(server_sock,
                                 (struct sockaddr *)&client_addr,
                                 &client_len);

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("%s on port %d...\n", client_ip, port);


        /* Doc xau chao tu file va gui cho client */
        FILE *fhello = fopen(hello_file, "r");
        char hello_buf[BUF_SIZE];
        int  hello_len = fread(hello_buf, 1, sizeof(hello_buf), fhello);
        fclose(fhello);

        send(client_sock, hello_buf, hello_len, 0);

        FILE *flog = fopen(log_file, "a");
        char buf[BUF_SIZE];
        int  received = 0;

        while ((received = recv(client_sock, buf, sizeof(buf), 0)) > 0) {
            fwrite(buf, 1, received, flog);
            fflush(flog);
        }

        fclose(flog);
        close(client_sock);
    }

    close(server_sock);
    return 0;
}