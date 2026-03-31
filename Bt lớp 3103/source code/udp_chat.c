
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Usage: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int   port_s = atoi(argv[1]);
    char *ip_d   = argv[2];
    int   port_d = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    unsigned long ul = 1;
    ioctl(sock, FIONBIO, &ul);

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_port        = htons((uint16_t)port_s);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port   = htons((uint16_t)port_d);
    inet_pton(AF_INET, ip_d, &dest_addr.sin_addr);

    printf("Connected: port %d forward to: %s:%d\n",
           port_s, ip_d, port_d);
    printf("Terminal Chat:\n");

    char buf[BUF_SIZE];

    while (1) {

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);  
        FD_SET(sock, &read_fds);          

        int max_fd = sock;  

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {

            int n = read(STDIN_FILENO, buf, BUF_SIZE - 1);
            if (n <= 0) break;

            if (buf[n - 1] == '\n') {
                buf[--n] = '\0';
            }

            sendto(sock, buf, n, 0,
                   (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        }

        if (FD_ISSET(sock, &read_fds)) {

            struct sockaddr_in src_addr;
            socklen_t src_len = sizeof(src_addr);

            int n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
                             (struct sockaddr *)&src_addr, &src_len);

            if (n > 0) {
                buf[n] = '\0';

                char src_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &src_addr.sin_addr, src_ip, sizeof(src_ip));

                printf("[%s:%d] %s\n",
                       src_ip, ntohs(src_addr.sin_port), buf);
                fflush(stdout);
            }
        }
    }

    close(sock);
    return 0;
}
