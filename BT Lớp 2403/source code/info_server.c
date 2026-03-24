#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>     
#include <unistd.h>      
#include <sys/socket.h>  
#include <arpa/inet.h>  
#include <netinet/in.h>  

#define MAX_PATH  4096
#define MAX_NAME  256

int recv_all(int sock, void *buf, int len) {
    int total = 0;
    char *p = (char *)buf;

    while (total < len) {
        int received = recv(sock, p + total, len - total, 0);
        if (received <= 0) {
            return -1;
        }
        total += received;
    }
    return 0;
}

int recv_uint8(int sock, uint8_t *out) {
    return recv_all(sock, out, 1);
}

int recv_uint16(int sock, uint16_t *out) {
    uint16_t net_val = 0;
    int ret = recv_all(sock, &net_val, 2);
    if (ret < 0) return -1;
    *out = ntohs(net_val);
    return 0;
}

int recv_uint32(int sock, uint32_t *out) {
    uint32_t net_val = 0;
    int ret = recv_all(sock, &net_val, 4);
    if (ret < 0) return -1;
    *out = ntohl(net_val); 
    return 0;
}

void handle_client(int client_sock, char *client_ip) {
    int      ret = 0;
    uint16_t path_len   = 0;
    uint16_t file_count = 0;
    uint32_t total_size = 0;       
    char     dir_name[MAX_PATH + 1];

    ret = recv_uint16(client_sock, &path_len);
    if (ret < 0) goto error;
    ret = recv_all(client_sock, dir_name, (int)path_len);
    if (ret < 0) goto error;
    dir_name[path_len] = '\0';
    printf(" Thu muc: %s\n\n", dir_name);

    ret = recv_uint16(client_sock, &file_count);
    if (ret < 0) goto error;
    for (uint32_t i = 0; i < file_count; i++) {

        uint8_t  name_len = 0;
        char     name[MAX_NAME + 1];
        uint32_t file_size = 0;

        ret = recv_uint8(client_sock, &name_len);
        if (ret < 0) goto error;

        ret = recv_all(client_sock, name, (int)name_len);
        if (ret < 0) goto error;
        name[name_len] = '\0';

        ret = recv_uint32(client_sock, &file_size);
        if (ret < 0) goto error;

        printf(" %-36s %12u byte\n", name, file_size);

        total_size += file_size;
    }

    fflush(stdout);
    return;

error:
    printf("ERROR %s\n", client_ip);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int ret  = 0;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        return 1;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY; 

    ret = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        close(server_sock);
        return 1;
    }

    ret = listen(server_sock, 5);  
    if (ret < 0) {
        close(server_sock);
        return 1;
    }

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char client_ip[INET_ADDRSTRLEN];

        int client_sock = accept(server_sock,
                                 (struct sockaddr *)&client_addr,
                                 &client_len);
        if (client_sock < 0) {
            continue; 
        }

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        handle_client(client_sock, client_ip);
        close(client_sock);
    }

    close(server_sock);
    return 0;
}