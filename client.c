/*
-> client can connect using different machine by changing to appropriate IP
address

TODO:
-> find a better way of reading/writing from/to server
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

// port over which connection is established
#define PORT 8080

int main() {
  // buffer for reading/writing from/to server
  char buffer[100];
  // n is just a temporary variable to do read/write from/to server
  // stop is used to break out of the read/write loop when EXIT command is
  // issued
  int n, stop = 0;
  // to store server address information
  struct sockaddr_in servaddr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Socket connection failed...\n");
    return 0;
  }
  printf("Socket connection succeeded...\n");
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);
  // attempt to connect to server
  if ((connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
    printf("Connection to server failed...\n");
    return 0;
  }
  printf("Connected to server...\n");
  while (stop == 0) {
    bzero(buffer, sizeof(buffer));
    // take input from user
    printf("Enter the command: ");
    n = 0;
    while ((buffer[n++] = getchar()) != '\n')
      ;
    // if EXIT command issued, exit from the loop by setting flag variable stop
    // to 1
    if (strncmp("EXIT", buffer, 4) == 0) {
      stop = 1;
    }
    // write to server
    write(sockfd, buffer, sizeof(buffer));
    // set buffer to binary zeroes to clear any unwanted data in next iteration
    bzero(buffer, sizeof(buffer));
    // read from server
    read(sockfd, buffer, sizeof(buffer));
    printf("%s", buffer);
  }
  close(sockfd);
  return 0;
}
