#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("Server is ready and waiting for messages...\n");

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
        // קריאת ההודעה מהלקוח
        int valread = recv(new_socket, buffer, 1024, 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("Client sent: %s\n", buffer);

            // יצירת תשובה מותאמת
            char response[1100];
            sprintf(response, "Server received your message: '%s'. Nice to meet you!", buffer);
            
            send(new_socket, response, strlen(response), 0);
        }

        close(new_socket);
        memset(buffer, 0, sizeof(buffer)); // ניקוי הבאפר ללקוח הבא
    }

    close(server_fd);
    return 0;
}