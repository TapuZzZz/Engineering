#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define IP_ADDRESS "0.0.0.0"
#define PORT 5008
#define MAXLINE 1024

int main()
{
    int server_fd, client_fd, n;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[MAXLINE];
	char *echo_str = "Server reply: ";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    servaddr.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Accept an incoming connection
    socklen_t len = sizeof(cliaddr);
    if ((client_fd = accept(server_fd, (struct sockaddr *)&cliaddr, &len)) < 0) {
         perror("accept");
         exit(EXIT_FAILURE);
    }

    while (1) {
        
        // Receive data from the client
        while ((n = read(client_fd, buffer, MAXLINE)) > 0) {
            //strcat(echo_str, buffer);
			//send(client_fd, echo_str, sizeof(echo_str), 0); // Echo back the data
			send(client_fd, buffer, sizeof(buffer), 0); // Echo back the data
			printf("\n client sent: %s", buffer);
			//printf("\n server sent: %s", echo_str);
			memset(buffer, 0, MAXLINE);
        }

        if (n < 0) {
            perror("read");
        }

        
    }
    close(client_fd); 
	close(server_fd);
    return 0;
}