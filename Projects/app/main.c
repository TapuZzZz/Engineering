#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define IP "127.0.0.1"
#define PORT 8888

int main() {
    int s;
    struct sockaddr_in sock;
    char buffer[1024] = {0};
    char message[512];

    s = socket(AF_INET, SOCK_STREAM, 0);
    sock.sin_family = AF_INET;
    sock.sin_port = htons(PORT);
    sock.sin_addr.s_addr = inet_addr(IP);

    if (connect(s, (struct sockaddr*)&sock, sizeof(sock)) < 0) {
        perror("Connect failed");
        return 1;
    }

    // קלט מהמשתמש
    printf("Enter a message to send to the server: ");
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\n")] = 0; // הורדת ה-Enter מהסוף

    send(s, message, strlen(message), 0);
    
    int valread = recv(s, buffer, 1024, 0);
    if (valread > 0) {
        printf("\nMessage from server: %s\n", buffer);
    }

    close(s);
    return 0;
}