//  FTP Sever 
//=============
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h> // FIXED: Added for 'bool' type support
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>

#define CONTROL_PORT 21
#define BUFFER_SIZE 1024

#define ALLOWED_USER "my_user"
#define ALLOWED_PASS "my_secret_pass"

// מבנה נתונים המייצג את מצב הלקוח הנוכחי בשרת
typedef struct {
    char username[50];
    bool is_logged_in;
} FTPSession;

// מבנה נתונים לניהול מצב התקשורת של הלקוח
typedef struct {
    int control_sock;
    int pasv_listen_sock; // משמש במצב פסיבי בלבד
    char data_ip[16];     // IP של הלקוח למצב אקטיבי
    int data_port;        // פורט של הלקוח למצב אקטיבי
    int is_passive;       // דגל מסמן מצב פסיבי (1) או אקטיבי (0)
} FTPState;

//  send_response()
// ===================
void send_response(int sock, const char *code_str) {
    send(sock, code_str, strlen(code_str), 0);
}

//  handle_quit()
// ===================
// Handles graceful FTP session teardown and resource cleanup
void handle_quit(FTPState *state, FTPSession *session) {
    send_response(state->control_sock, "221 Goodbye. Service closing control connection.\r\n");
    
    // Reset session authentication variables
    memset(session->username, 0, sizeof(session->username));
    session->is_logged_in = false;
    
    // Clean up any dangling passive listener sockets
    if (state->pasv_listen_sock >= 0) {
        close(state->pasv_listen_sock);
        state->pasv_listen_sock = -1;
    }
    state->is_passive = 0;
    
    printf("Client requested QUIT. Session closed gracefully.\n");
}

//  open_data_connection()
// =========================
int open_data_connection(FTPState *state) {
    if (state->is_passive) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int data_sock = accept(state->pasv_listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        close(state->pasv_listen_sock); // סגירת ה-Listening socket
        state->pasv_listen_sock = -1;
        state->is_passive = 0;
        return data_sock;
    } else {
        int data_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sock < 0) return -1;

        struct sockaddr_in data_addr;
        memset(&data_addr, 0, sizeof(data_addr));
        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(state->data_port);
        inet_pton(AF_INET, state->data_ip, &data_addr.sin_addr);

        if (connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
            close(data_sock);
            return -1;
        }
        return data_sock;
    }
}

//  handle_port()
// ===================
void handle_port(FTPState *state, char *args) {
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(args, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
        snprintf(state->data_ip, sizeof(state->data_ip), "%d.%d.%d.%d", h1, h2, h3, h4);
        state->data_port = p1 * 256 + p2;
        state->is_passive = 0;
        send_response(state->control_sock, "200 PORT command successful.\r\n");
    } else {
        send_response(state->control_sock, "501 Syntax error in parameters.\r\n");
    }
}

//  handle_pasv()
// ===================
void handle_pasv(FTPState *state) {
    state->pasv_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in pasv_addr;
    memset(&pasv_addr, 0, sizeof(pasv_addr));
    pasv_addr.sin_family = AF_INET;
    pasv_addr.sin_addr.s_addr = INADDR_ANY;
    pasv_addr.sin_port = 0; 
    
    bind(state->pasv_listen_sock, (struct sockaddr *)&pasv_addr, sizeof(pasv_addr));
    listen(state->pasv_listen_sock, 1);

    socklen_t len = sizeof(pasv_addr);
    getsockname(state->pasv_listen_sock, (struct sockaddr *)&pasv_addr, &len);
    int port = ntohs(pasv_addr.sin_port);

    int p1 = port / 256;
    int p2 = port % 256;
    char response[128];
    snprintf(response, sizeof(response), "227 Entering Passive Mode (127,0,0,1,%d,%d).\r\n", p1, p2);
    
    state->is_passive = 1;
    send_response(state->control_sock, response);
}

//  handle_list()
//======================
void handle_list(FTPState *state) {
    send_response(state->control_sock, "150 Here comes the directory listing.\r\n");

    int data_sock = open_data_connection(state);
    if (data_sock < 0) {
        send_response(state->control_sock, "425 Can't open data connection.\r\n");
        return;
    }

    DIR *d = opendir(".");
    struct dirent *dir;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char buffer[256];
            int len = snprintf(buffer, sizeof(buffer), "%s\r\n", dir->d_name);
            send(data_sock, buffer, len, 0);
        }
        closedir(d);
    }

    close(data_sock); 
    send_response(state->control_sock, "226 Directory send OK.\r\n");
}

//  handle_retr()
// ==============
void handle_retr(FTPState *state, char *filename) {
    filename[strcspn(filename, "\r\n")] = 0;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        send_response(state->control_sock, "550 File not found.\r\n");
        return;
    }

    send_response(state->control_sock, "150 Opening BINARY mode data connection.\r\n");

    int data_sock = open_data_connection(state);
    if (data_sock < 0) {
        send_response(state->control_sock, "425 Can't open data connection.\r\n");
        fclose(file);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(data_sock, buffer, bytes_read, 0);
    }

    fclose(file);
    close(data_sock); 
    send_response(state->control_sock, "226 Transfer complete.\r\n");
}

//  MAIN
// ======
int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // 1. יצירת Socket לערוץ הבקרה (Control Socket)
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. הגדרת כתובת השרת ומשאב ה-Port (21)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CONTROL_PORT);

    // 3. ביצוע Bind לפורט 21
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 4. מעבר למצב האזנה (Listen)
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("FTP Server running on port %d...\n", CONTROL_PORT);

    // 5. קבלת חיבור נכנס בודד בלולאה (Iterative Server)
    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) >= 0) {
        
        // Init Session and State
        FTPState state = { .control_sock = client_sock, .pasv_listen_sock = -1, .is_passive = 0 };
        FTPSession session = { .username = {0}, .is_logged_in = false };
        
        send_response(client_sock, "220 Simple FTP Server Ready.\r\n");

        // לולאת טיפול בפקודות הבקרה
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) break; // הלקוח התנתק באובדן תקשורת

            printf("Command Received: %s", buffer);
            
            // Global check for QUIT command regardless of auth state
            if (strncmp(buffer, "QUIT", 4) == 0) {
                handle_quit(&state, &session);
                break; // Break the command loop to gracefully disconnect client
            }
            
            // 1. Allow unauthenticated access ONLY to login commands
            // -----------------------------------------------------------
            if (!session.is_logged_in) {
                if (strncmp(buffer, "USER", 4) == 0) {
                    if (sscanf(buffer + 5, "%49s", session.username) == 1) {
                        send_response(client_sock, "331 User name okay, need password.\r\n");
                    } else {
                        send_response(client_sock, "501 Syntax error in parameters.\r\n");
                    }
                }
                else if (strncmp(buffer, "PASS", 4) == 0) { 
                    char password[50] = {0};
                    if (sscanf(buffer + 5, "%49s", password) == 1) {
                        if (strcmp(session.username, ALLOWED_USER) == 0 && strcmp(password, ALLOWED_PASS) == 0) {
                            session.is_logged_in = true;
                            send_response(client_sock, "230 User logged in, proceed.\r\n");
                        } else {
                            session.is_logged_in = false;
                            send_response(client_sock, "530 Not logged in. Invalid username or password.\r\n");
                        }
                    } else {
                        send_response(client_sock, "501 Syntax error in parameters.\r\n");
                    }
                }
                else {
                    send_response(client_sock, "530 Please login with USER and PASS.\r\n");
                }
            } 
            // 2. Client IS authenticated - process data and file transfer commands
            // -----------------------------------------------------------
            else {
                if (strncmp(buffer, "PASV", 4) == 0) {
                    handle_pasv(&state);
                }
                else if (strncmp(buffer, "PORT", 4) == 0) {
                    handle_port(&state, buffer + 5);
                }
                else if (strncmp(buffer, "LIST", 4) == 0) {
                    handle_list(&state);
                }
                else if (strncmp(buffer, "RETR", 4) == 0) {
                    char filename[256] = {0};
                    if (sscanf(buffer + 5, "%255s", filename) == 1) {
                        handle_retr(&state, filename);
                    } else {
                        send_response(client_sock, "501 Syntax error in parameters.\r\n");
                    }
                }
                else {
                    send_response(client_sock, "502 Command not implemented.\r\n");
                }
            }
        }

        close(client_sock);
    }

    close(server_sock);
    return 0;
}