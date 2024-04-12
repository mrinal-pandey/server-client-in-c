/*
-> test cases have been run
-> ASAN/valgrind have been run on the code

TODO:
-> add a more efficient way to read/write from/to client (current method uses
buffer to read/write)
-> make server capable of exiting by issuing EXIT command at server (presently
we have to Ctrl+C the server to stop)
-> find an optimal way to free the deleted node from the memory in function
remove_from_list()
-> more optimized way to process the input from the client using lex/flex and
yacc/bison (currently manual processing is done)
*/

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

// port over which connection is established
#define PORT 8080
// MAX number of characters that can be read/write from/to client
#define MAX 100

// structure to store client information
struct clients {
  struct sockaddr_in cliaddr;
  int addrlen;
  int connfd;
} client[1000];

// a node of the list
typedef struct linked_list {
  int data;
  struct linked_list *next;
} list;

// head of the list
list *head;
// lock used for concurrency issues
pthread_mutex_t lock;

// search through the list to find a given number
// return 1 if found, 0 if not found
int search(int number) {
  list *temp = head;
  int found = 0;
  while (temp != NULL) {
    if (temp->data == number) {
      found = 1;
    }
    temp = temp->next;
  }
  return found;
}

// create a new node for the list
list *create_new_node(int number) {
  list *new_node = (list *)malloc(sizeof(list));
  new_node->data = number;
  new_node->next = NULL;
  return new_node;
}

// display the list elements in format [data]-->NULL
char *display_list() {
  list *temp = head;
  char *elements = (char *)malloc(sizeof(char));
  char *temp_element = (char *)malloc(sizeof(char));
  if (temp == NULL) {
    printf("List is empty...\n");
    return "List is empty...\n";
  }
  while (temp != NULL) {
    strcat(elements, "[");
    sprintf(temp_element, "%d", temp->data);
    strcat(elements, temp_element);
    strcat(elements, "]");
    strcat(elements, "--->");
    temp = temp->next;
  }
  strcat(elements, "NULL\n");
  printf("%s", elements);
  return elements;
}

// add an element to the list
char *add_to_list(int number) {
  // apply a lock before attempting to add to the list to prevent concurrency
  // issues
  pthread_mutex_lock(&lock);
  list *temp;
  if (head == NULL) {
    // add first element to the list
    head = create_new_node(number);
  } else {
    // if given number already exists, avoid adding to the list
    if (search(number) == 1) {
      printf("Number already exists, aborting add operation...\n");
      // unlock before exiting
      pthread_mutex_unlock(&lock);
      return "Number already exists, aborting add operation...\n";
    } else {
      // to add a number to list if at least one element is already present in
      // the list
      temp = head;
      while (temp->next != NULL) {
        temp = temp->next;
      }
      temp->next = create_new_node(number);
    }
  }
  printf("Added to list...\n");
  // remove the lock before exiting
  pthread_mutex_unlock(&lock);
  return "Added to list...\n";
}

// remove an element from the list
char *remove_from_list(int number) {
  // apply a lock before attempting to remove from list to provide concurrency
  // issues
  pthread_mutex_lock(&lock);
  list *forward, *behind, *temp;
  int stop = 0;
  if (head == NULL) {
    printf("List is empty...\n");
    // unlock before exiting
    pthread_mutex_unlock(&lock);
    return "List is empty...\n";
  }
  // check if the number is present, if yes, attempt to remove it from the list
  // else unlock mutex and exit
  if (search(number) != 1) {
    printf("Number doesn't exist...\n");
    pthread_mutex_unlock(&lock);
    return "Number doesn't exist...\n";
  }
  // remove an element from list and break out of loop using flag variable stop
  forward = head;
  behind = NULL;
  while (stop == 0 && forward != NULL) {
    if (forward->data == number) {
      temp = forward;
      if (behind == NULL) {
        head = forward->next;
      } else {
        behind->next = forward->next;
      }
      stop = 1;
    }
    behind = forward;
    forward = forward->next;
  }
  printf("Removed from list...\n");
  // unlock before exiting
  pthread_mutex_unlock(&lock);
  return "Removed from list...\n";
}

// to check which operation is to performed
char *perform_operation(char operation[5], int number) {
  if (strcmp(operation, "ADD") == 0 && number != -1) {
    return add_to_list(number);
  } else if (strcmp(operation, "DEL") == 0 && number != -1) {
    return remove_from_list(number);
  } else if (strcmp(operation, "SHOW") == 0 && number == -1) {
    return display_list();
  } else {
    printf("Wrong Input... Please provide a valid command...\n");
    return "Wrong Input... Please provide a valid command...\n";
  }
  return "\n";
}

// separate the input received from the client into the command and number
// check if ADD/DEL is followed by a number and SHOW is not
char *process_command(char command[100]) {
  int i = 0, j = 0, number = -1, multiplier = 1;
  char operation[5];
  while (command[i] != ' ' && command[i] != '\0' && command[i] != '\n') {
    operation[i] = command[i];
    i++;
  }
  operation[i] = '\0';
  i++;
  j = i;
  while (command[j] != '\0' && command[j] != '\n') {
    j++;
    multiplier *= 10;
    number = 0;
  }
  multiplier /= 10;
  while (command[i] != '\0' && command[i] != '\n') {
    number += multiplier * ((int)(command[i]) - 48);
    multiplier /= 10;
    i++;
  }
  return perform_operation(operation, number);
}

// to read the input from client and write the output back to it
void *get_command(void *args) {
  int n, stop = 0;
  char buffer[MAX];
  char *response;
  int connfd = *((int *)args);
  while (stop == 0) {
    bzero(buffer, MAX);
    // read from client
    read(connfd, buffer, sizeof(buffer));
    printf("Command received: %s", buffer);
    // if client issues EXIT command, exit without any further processing
    if (strncmp("EXIT", buffer, 4) == 0) {
      stop = 1;
      printf("A client disconnected...\n");
    } else {
      response = process_command(buffer);
    }
    bzero(buffer, MAX);
    if (stop == 1) {
      strcpy(buffer, "Exiting...\n");
    } else {
      strcpy(buffer, response);
    }
    // write to the client
    write(connfd, buffer, sizeof(buffer));
  }
  return NULL;
}

int main() {
  int client_number = 0;
  pthread_t threads[1000];
  int sockfd;
  struct sockaddr_in servaddr;
  // create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Error in connecting to socket...\n");
    return 1;
  }
  printf("Socket created successfully...\n");
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);
  // bind the socket
  if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
    printf("Error in binding socket...\n");
    return 1;
  }
  printf("Socket binded successfully...\n");
  // initialize the lock
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("Mutex init has failed...\n");
    return 1;
  }
  // listen for connections
  if ((listen(sockfd, 5)) != 0) {
    printf("Listen failed...\n");
  } else {
    printf("Listening...\n\n");
    while (1) {
      client[client_number].addrlen = sizeof(client[client_number].cliaddr);
      // accept the connection from client
      client[client_number].connfd =
          accept(sockfd, (struct sockaddr *)&client[client_number].cliaddr,
                 (socklen_t *)&client[client_number].addrlen);
      printf("Client %d connected...\n\n", (client_number + 1));
      // create a separate pthread to serve any client that connects
      pthread_create(&threads[client_number], NULL, get_command,
                     &client[client_number].connfd);
      client_number++;
    }
    int j;
    // join all the threads
    for (j = 0; j <= client_number; j++) {
      pthread_join(threads[j], NULL);
    }
  }
  close(sockfd);
  pthread_mutex_destroy(&lock);
  return 0;
}
