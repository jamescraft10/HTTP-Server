#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip.h>

#define PORT 3000
#define BUFFER_SIZE 1024

int main(void) {
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

        // receive request
        char *path;
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
                    path = path_start+4;
                }
            }
        }

        // Get res info then put it in a real http response
        char res_body[BUFFER_SIZE];
        strcpy(res_body, path);
        char res_body_len[10];
        snprintf(res_body_len, sizeof(res_body_len), "%lu", strlen(res_body));

        char res_http_code[] = "200 OK";
        char res_content_type[] = "text/plain";

        char http_response[BUFFER_SIZE];
        snprintf(http_response, sizeof(http_response), "HTTP/1.1 %s\r\n"
                                                    "Content-Type: %s\r\n"
                                                    "Content-Length: %s\r\n"
                                                    "\r\n"
                                                    "%s",
                                                    res_http_code,
                                                    res_content_type,
                                                    res_body_len,
                                                    res_body);


        ssize_t sent = send(cfd, (void *)http_response, strlen(http_response), 0);
        close(cfd);
    }

    // Cleanup
    close(sfd);
    return 0;
}