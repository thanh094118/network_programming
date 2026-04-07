#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CLIENTS  10
#define BUF_SIZE     1024
#define NAME_SIZE    64

#define STATE_WAIT_ID    0   
#define STATE_READY      1   

struct Client {
    int  fd;
    int  state;
    char id[NAME_SIZE];   
    char name[NAME_SIZE];  
};

void get_timestamp(char *buf, int len) {
    time_t     t  = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y/%m/%d %I:%M:%S%p", tm);
}

void send_str(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

void broadcast(struct Client *clients, int num, int sender_fd, const char *msg) {
    for (int i = 0; i < num; i++) {
        if (clients[i].fd != sender_fd && clients[i].state == STATE_READY) {
            send_str(clients[i].fd, msg);
        }
    }
}

int parse_id_name(const char *buf, char *id, char *name) {
    const char *colon = strchr(buf, ':');
    if (colon == NULL) return 0;

    int id_len = colon - buf;
    if (id_len == 0 || id_len >= NAME_SIZE) return 0;
    strncpy(id, buf, id_len);
    id[id_len] = '\0';

    const char *p = colon + 1;
    while (*p == ' ') p++;
    if (*p == '\0') return 0;
    strncpy(name, p, NAME_SIZE - 1);
    name[NAME_SIZE - 1] = '\0';

    for (int i = 0; id[i]; i++) {
        if (id[i] == ' ') return 0;
    }
    for (int i = 0; name[i]; i++) {
        if (name[i] == ' ') return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 5);

    printf("Connected port %d...\n", port);

    struct Client clients[MAX_CLIENTS];
    int           num_clients = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    while (1) {

        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(listener, &read_fds);
        int max_fd = listener;

        for (int i = 0; i < num_clients; i++) {
            FD_SET(clients[i].fd, &read_fds);
            if (clients[i].fd > max_fd) {
                max_fd = clients[i].fd;
            }
        }

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            break;
        }

        if (FD_ISSET(listener, &read_fds)) {

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(listener,
                                   (struct sockaddr *)&client_addr,
                                   &client_len);

            if (client_fd >= 0 && num_clients < MAX_CLIENTS) {

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr,
                          client_ip, sizeof(client_ip));
                printf("Client: %s (fd=%d)\n", client_ip, client_fd);

                clients[num_clients].fd    = client_fd;
                clients[num_clients].state = STATE_WAIT_ID;
                memset(clients[num_clients].id,   0, NAME_SIZE);
                memset(clients[num_clients].name, 0, NAME_SIZE);
                num_clients++;

                send_str(client_fd,
                         "Connected.\n"
                         "  client_id: client_name\n");
            }
        }

        for (int i = 0; i < num_clients; i++) {

            if (!FD_ISSET(clients[i].fd, &read_fds)) {
                continue;
            }

            char buf[BUF_SIZE];
            int  n = recv(clients[i].fd, buf, BUF_SIZE - 1, 0);

            if (n <= 0) {
                printf("fd=%d (%s)\n",
                       clients[i].fd,
                       clients[i].state == STATE_READY ? clients[i].id : "ERROR");

                if (clients[i].state == STATE_READY) {
                    char notice[BUF_SIZE];
                    snprintf(notice, BUF_SIZE,
                             "%s\n",
                             clients[i].id);
                    broadcast(clients, num_clients, clients[i].fd, notice);
                }

                close(clients[i].fd);
                clients[i] = clients[--num_clients];
                i--;
                continue;
            }

            buf[n] = '\0';

            while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) {
                buf[--n] = '\0';
            }

            if (clients[i].state == STATE_WAIT_ID) {

                char id[NAME_SIZE], name[NAME_SIZE];

                if (parse_id_name(buf, id, name)) {

                    strncpy(clients[i].id,   id,   NAME_SIZE - 1);
                    strncpy(clients[i].name, name, NAME_SIZE - 1);
                    clients[i].state = STATE_READY;

                    printf("registered: id=%s name=%s (fd=%d)\n",
                           id, name, clients[i].fd);

                    char welcome[BUF_SIZE];
                    snprintf(welcome, BUF_SIZE,
                             "Done! %s (%s)\n", name, id);
                    send_str(clients[i].fd, welcome);

                    char notice[BUF_SIZE];
                    snprintf(notice, BUF_SIZE,
                             "%s (%s) registered\n", name, id);
                    broadcast(clients, num_clients, clients[i].fd, notice);

                } else {
                    send_str(clients[i].fd,
                             "Syntax error!\n"
                             "client_id: client_name\n");
                }

            } else if (clients[i].state == STATE_READY) {

                char timestamp[32];
                get_timestamp(timestamp, sizeof(timestamp));

                char msg[BUF_SIZE * 2];
                snprintf(msg, sizeof(msg), "%s %s: %s\n",
                         timestamp, clients[i].id, buf);

                printf(" %s", msg);
                broadcast(clients, num_clients, clients[i].fd, msg);
            }
        }
    }

    close(listener);
    return 0;
}
