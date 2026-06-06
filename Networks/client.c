#include <stdio.h>
//#include <stdlib.h> 
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT_NO 5008
#define IP_ADDRESS "127.0.0.111"
#define MAX_BUFFER_SIZE 1024

int main()
{
	int sockfd;
	struct sockaddr_in server_addr;
	int bytes_sent;
	
	char input[MAX_BUFFER_SIZE];
	char from_server[MAX_BUFFER_SIZE];
	int n;
	
	printf("Client is up....\n");
	
	/* Create socke */
	/*--------------*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
       perror("Error creating socket");
       return 1;
	}
	
	/*init server addr and connect */
	/*-----------------------------*/
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NO);
	server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	  if(connect(sockfd, (struct sockaddr*)&server_addr,sizeof(server_addr)) == -1)
	  {
	   perror("Error connecting to server");
	   return 1;
	  }
	 
	 
	
	/* Get msg from user and send it to server */
	/*-----------------------------------------*/
	do
	{
	   printf("Enter a message (or 'Exit' to quit): ");
	   fgets(input, sizeof(input), stdin);
	   //input[strlen(input) - 1] = '\0';
	   bytes_sent = send(sockfd, input, strlen(input), 0);
	   if (bytes_sent == -1)
	   {
	     perror("Error sending data");
	     close(sockfd);
	     return 1;
	   }
	   n = read(sockfd, from_server, MAX_BUFFER_SIZE);
	   printf("%s \n", from_server);
	 }
	 while (strcmp(input, "Exit") != 0);
	 close(sockfd);
	 return 0;
}