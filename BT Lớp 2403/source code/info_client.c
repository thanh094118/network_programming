#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>      
#include <unistd.h>    
#include <dirent.h>    
#include <sys/stat.h>   
#include <sys/socket.h>  
#include <arpa/inet.h>   
#include <netinet/in.h>  

#define MAX_FILES  10
#define MAX_PATH   4096
#define MAX_NAME   256

struct FileInfo {
    char     name[MAX_NAME];  
    uint32_t size;          
};

int send_all(int sock, void *buf, int len) {
    int total = 0;
    char *p = (char *)buf;

    while (total < len) {
        int sent = send(sock, p + total, len - total, 0);
        if (sent <= 0) {
            return -1;
        }
        total += sent;
    }
    return 0;
}

int send_uint8(int sock, uint8_t val) {
    return send_all(sock, &val, 1);
}
int send_uint16(int sock, uint16_t val) {
    uint16_t net_val = htons(val);  
    return send_all(sock, &net_val, 2);
}
int send_uint32(int sock, uint32_t val) {
    uint32_t net_val = htonl(val);  
    return send_all(sock, &net_val, 4);
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        return 1;
    }

    char   *server_ip  = argv[1];
    int     port       = atoi(argv[2]);
    int     ret        = 0; 

    char cwd[MAX_PATH];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return 1;
    }

    printf("Folder: %s\n", cwd);

    struct FileInfo files[MAX_FILES];
    uint16_t        file_count = 0;

    DIR            *dir   = NULL;
    struct dirent  *entry = NULL;
    struct stat     st;
    char            full_path[MAX_PATH + MAX_NAME + 2];
    dir = opendir(cwd);
    if (dir == NULL) {
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {

        if (entry->d_name[0] == '.') {
            continue;
        }

        if (file_count >= MAX_FILES) {
            break;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, entry->d_name);
        if (stat(full_path, &st) != 0) {
            continue; 
        }
        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        strncpy(files[file_count].name, entry->d_name, MAX_NAME - 1);
        files[file_count].name[MAX_NAME - 1] = '\0';
        files[file_count].size = (uint32_t)st.st_size;
        file_count++;
    }

    closedir(dir);

    for (int i = 0; i < file_count; i++) {
        printf(" %d %s - %u byte\n", i + 1, files[i].name, files[i].size);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 1;
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons((uint16_t)port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        close(sock);
        return 1;
    }

    ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        close(sock);
        return 1;
    }

    uint16_t path_len = (uint16_t)strlen(cwd);
    ret = send_uint16(sock, path_len);
    if (ret < 0) goto error;
    ret = send_all(sock, cwd, (int)path_len);
    if (ret < 0) goto error;

    ret = send_uint16(sock, file_count);
    if (ret < 0) goto error;

    for (int i = 0; i < file_count; i++) {
        uint8_t name_len = (uint8_t)strlen(files[i].name);

        ret = send_uint8(sock, name_len);
        if (ret < 0) goto error;

        ret = send_all(sock, files[i].name, (int)name_len);
        if (ret < 0) goto error;

        ret = send_uint32(sock, files[i].size);
        if (ret < 0) goto error;
    }

    printf("Done.\n");
    close(sock);
    return 0;

error:
    close(sock);
    return 1;
}