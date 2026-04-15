#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CLIENTS  10
#define BUF_SIZE     1024
#define NAME_SIZE    64

#define STATE_WAIT_ID 0
#define STATE_READY   1

struct Client {
    int  fd;
    int  state;
    char id[NAME_SIZE];
    char name[NAME_SIZE];
};

/* ================== Utility ================== */

void get_timestamp(char *buf, int len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y/%m/%d %H:%M:%S", tm);
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
    if (!colon) return 0;

    int id_len = colon - buf;
    if (id_len == 0 || id_len >= NAME_SIZE) return 0;

    strncpy(id, buf, id_len);
    id[id_len] = '\0';

    const char *p = colon + 1;
    while (*p == ' ') p++;

    if (*p == '\0') return 0;

    strncpy(name, p, NAME_SIZE - 1);
    name[NAME_SIZE - 1] = '\0';

    return 1;
}

/* ================== MAIN ================== */

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listener, 5);

    printf("[Server] poll() running on port %d...\n", port);

    struct Client clients[MAX_CLIENTS];
    struct pollfd fds[MAX_CLIENTS + 1]; // +1 cho listener

    int nfds = 1; // số fd dùng cho poll

    // fd đầu tiên là listener
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    while (1) {

        // 🔥 KHÁC BIỆT 1: poll KHÔNG cần rebuild toàn bộ như select
        int ret = poll(fds, nfds, -1);

        if (ret < 0) {
            perror("poll error");
            break;
        }

        /* ===== CLIENT MỚI ===== */
        if (fds[0].revents & POLLIN) {

            int client_fd = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS + 1) {
                int idx = nfds - 1;

                clients[idx].fd = client_fd;
                clients[idx].state = STATE_WAIT_ID;

                // thêm vào poll
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;

                printf("New client fd=%d\n", client_fd);

                send_str(client_fd,
                    "Nhap: client_id: client_name\n> ");
            }
        }

        /* ===== XỬ LÝ CLIENT ===== */
        for (int i = 1; i < nfds; i++) {

            // 🔥 KHÁC BIỆT 2: poll dùng revents (không cần FD_ISSET)
            if (!(fds[i].revents & (POLLIN | POLLERR)))
                continue;

            char buf[BUF_SIZE];
            int n = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

            if (n <= 0) {
                printf("Client fd=%d disconnected\n", fds[i].fd);

                close(fds[i].fd);

                // 🔥 KHÁC BIỆT 3: xóa bằng cách dồn mảng
                fds[i] = fds[nfds - 1];
                clients[i - 1] = clients[nfds - 2];
                nfds--;
                i--;
                continue;
            }

            buf[n] = '\0';

            struct Client *c = &clients[i - 1];

            /* ===== REGISTER ===== */
            if (c->state == STATE_WAIT_ID) {

                char id[NAME_SIZE], name[NAME_SIZE];

                if (parse_id_name(buf, id, name)) {

                    strcpy(c->id, id);
                    strcpy(c->name, name);
                    c->state = STATE_READY;

                    send_str(c->fd, "OK\n");

                } else {
                    send_str(c->fd, "Sai cu phap\n");
                }
            }

            /* ===== CHAT ===== */
            else {

                char time_str[64];
                get_timestamp(time_str, sizeof(time_str));

                char msg[BUF_SIZE * 2];

                snprintf(msg, sizeof(msg),
                         "[%s] %s: %s",
                         time_str, c->id, buf);

                printf("%s", msg);

                broadcast(clients, nfds - 1, c->fd, msg);
            }
        }
    }

    close(listener);
    return 0;
}