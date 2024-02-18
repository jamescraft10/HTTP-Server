#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip.h>

#define BUFFER_SIZE 1024

char *path_to_code(char *path1) {
    if(path1[strlen(path1) - 1] == '/') { strcat(path1, "index.html"); }
    char path[sizeof("./public/")+sizeof(path1)] = "./public";
    strncat(path, path1, strlen(path1));
    printf("%s\n", path);

    FILE *fptr;
    fptr = fopen(path, "r");
    char *file_contents;

    // 404
    if(fptr == NULL) { // checks if the files doesnt exist
        FILE *fptr_404;
        fptr_404 = fopen("./public/404.html", "r");

        file_contents = NULL;
        char buffer[512];
        size_t total_size = 0;

        while(fgets(buffer, sizeof(buffer), fptr_404) != NULL) {
            size_t len = strlen(buffer);
            char *temp = realloc(file_contents, total_size + len + 1);
            if(temp == NULL) {
                perror("realloc failed :(\n");
                fclose(fptr_404);
                free(file_contents);
                return NULL;
            }

            file_contents = temp;
            strcpy(file_contents + total_size, buffer);
            total_size += len;
        }

        fclose(fptr_404);
        return file_contents;
    } else {
        file_contents = NULL;
        char buffer[512];
        size_t total_size = 0;

        while(fgets(buffer, sizeof(buffer), fptr) != NULL) {
            size_t len = strlen(buffer);
            char *temp = realloc(file_contents, total_size + len + 1);
            if(temp == NULL) {
                perror("realloc failed :(\n");
                fclose(fptr);
                free(file_contents);
                return NULL;
            }

            file_contents = temp;
            strcpy(file_contents + total_size, buffer);
            total_size += len;
        }

        fclose(fptr);
        return file_contents;
    }

    fclose(fptr);
    return file_contents;
}

char *path_to_type(char* path) {
    if(strcmp(&path[strlen(path)-5], ".html") == 0) {
        return "text/html";
    } else if(strcmp(&path[strlen(path)-4], ".css") == 0) {
        return "text/css";
    } else if(strcmp(&path[strlen(path)-3], ".js") == 0) {
        return "application/javascript";
    } else if(strcmp(&path[strlen(path)-3], ".ts") == 0) {
        return "application/typescript";
    } else {
        return "text/html";
    }
}

typedef struct res_struct response;
struct res_struct {
    char *http_code;
    char *http_type;
    char *file_code;
    size_t file_size;
};

typedef struct req_struct request;
struct req_struct {
    char *path;
};

int main(int argc, char *argv[]) {
    if(argc < 2) { printf("No port inputed\n"); return -1;}
    const int PORT = atoi(argv[1]);

    // create socket info stuff
    struct sockaddr_in server_info = {0};
    struct sockaddr client_info = {0};

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(PORT);

    socklen_t server_info_len = sizeof(server_info);
    socklen_t client_info_len = sizeof(client_info);

    // create socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > sfd) {
        perror("socket failed :(\n");
        return -1;
    }

    // bind
    if(0 > bind(sfd, (struct sockaddr*)&server_info, server_info_len)) { // i love c memory
        perror("bind failed :(\n");
        return -1;
    }

    // listen
    if(0 > listen(sfd, 0)) {
        perror("listen failed :(\n");
        return -1;
    }

    // listen
    char *username = getlogin();
    printf("Server is running on\n\thttp://localhost:%d/\n\thttp://127.0.0.1:%d/\n\thttp://%s:%d/\n\thttp://your-domain.com:%d/\n",PORT,PORT,username,PORT,PORT);
    while(1) {
        // accept
        int cfd = accept(sfd, &client_info, &client_info_len);
        if(0 > cfd) {
            perror("accept failed :(\n");
            return -1;
        }

        // request
        request req;
        char buffer[BUFFER_SIZE];
        ssize_t received = recv(cfd, buffer, sizeof(buffer) - 1, 0);
        if(received > 0) {
            buffer[received] = '\0';

            // print the path
            char *path_start = strstr(buffer, "GET ");
            if(path_start != NULL) {
                char *path_end = strchr(path_start+4, ' ');
                if(path_end != NULL) {
                    *path_end = '\0';
                    req.path = path_start+4;
                }
            }
        }

        // response
        response res;
        res.file_code = path_to_code(req.path);
        res.file_size = strlen(res.file_code);
        res.http_code = "200 OK"; // TODO: make function path_to_res_code(path)
        res.http_type = path_to_type(req.path);

        char http_response[BUFFER_SIZE];
        snprintf(http_response, sizeof(http_response), "HTTP/1.1 %s\r\n"
                                                   "Content-Type: %s\r\n"
                                                   "Content-Length: %zu\r\n" // %zu for size_t type
                                                   "\r\n"
                                                   "%s",
                                                   res.http_code,
                                                   res.http_type,
                                                   res.file_size,
                                                   res.file_code);

        ssize_t sent = send(cfd, (void *)http_response, strlen(http_response), 0);

        // Clean up
        close(cfd);
        free(res.file_code);
    }

    // Cleanup
    close(sfd);
    return 0;
}