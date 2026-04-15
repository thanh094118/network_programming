#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CLIENTS  10
#define BUF_SIZE     512
#define NAME_SIZE    64
#define DB_FILE      "users.txt"
#define OUT_FILE     "out.txt"

#define STATE_WAIT_USER   0
#define STATE_WAIT_PASS   1
#define STATE_LOGGED_IN   2

struct Client {
    int  fd;
    int  state;
    char username[NAME_SIZE];
};


void send_str(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

int check_login(const char *username, const char *password) {
    FILE *fp = fopen(DB_FILE, "r");
    if (!fp) return 0;

    char u[NAME_SIZE], p[NAME_SIZE];

    while (fscanf(fp, "%s %s", u, p) == 2) {
        if (strcmp(username, u) == 0 && strcmp(password, p) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

void exec_and_send(int fd, const char *cmd) {

    char full_cmd[BUF_SIZE];
    snprintf(full_cmd, sizeof(full_cmd), "%s > %s 2>&1", cmd, OUT_FILE);

    system(full_cmd);

    FILE *fp = fopen(OUT_FILE, "r");
    if (!fp) {
        send_str(fd, "Khong doc duoc file\n");
        return;
    }

    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), fp)) {
        send_str(fd, buf);
    }

    fclose(fp);
    send_str(fd, "\n> ");
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    printf("run on port %d\n", port);

    struct Client clients[MAX_CLIENTS];
    struct pollfd fds[MAX_CLIENTS + 1];

    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    while (1) {

        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {

            int client_fd = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS + 1) {

                int idx = nfds - 1;

                clients[idx].fd = client_fd;
                clients[idx].state = STATE_WAIT_USER;
                memset(clients[idx].username, 0, NAME_SIZE);

                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;

                printf("client fd=%d\n", client_fd);

                send_str(client_fd, "Username: ");
            }
        }

        for (int i = 1; i < nfds; i++) {

            if (!(fds[i].revents & (POLLIN | POLLERR)))
                continue;

            char buf[BUF_SIZE];
            int n = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

            if (n <= 0) {
                printf("%d disconnected\n", fds[i].fd);

                close(fds[i].fd);

                fds[i] = fds[nfds - 1];
                clients[i - 1] = clients[nfds - 2];
                nfds--;
                i--;
                continue;
            }

            buf[n] = '\0';

            while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
                buf[--n] = '\0';

            struct Client *c = &clients[i - 1];

            if (c->state == STATE_WAIT_USER) {

                strncpy(c->username, buf, NAME_SIZE - 1);
                c->state = STATE_WAIT_PASS;

                send_str(c->fd, "Password: ");
            }

            else if (c->state == STATE_WAIT_PASS) {

                if (check_login(c->username, buf)) {

                    c->state = STATE_LOGGED_IN;

                    send_str(c->fd,
                        "OK\n> ");
                } else {

                    send_str(c->fd,
                        "Sai \nUsername: ");

                    c->state = STATE_WAIT_USER;
                    memset(c->username, 0, NAME_SIZE);
                }
            }

            else {

                if (strcmp(buf, "exit") == 0) {
                    send_str(c->fd, "!\n");

                    close(c->fd);
                    fds[i] = fds[nfds - 1];
                    clients[i - 1] = clients[nfds - 2];
                    nfds--;
                    i--;
                    continue;
                }

                printf(" %s run: %s\n", c->username, buf);

                exec_and_send(c->fd, buf);
            }
        }
    }

    close(listener);
    return 0;
}