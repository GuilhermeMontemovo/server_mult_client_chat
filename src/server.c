#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#define MAX 4096 
#define PORT 8080 
#define SA struct sockaddr 
  

void *read_message(int *size){
    char **messages = calloc(1, sizeof(char *));
    messages[0] = calloc(MAX, sizeof(char));

    int n = 0;
    int i = 0;
    
    while ((messages[i][n++] = getchar()) != '\n'){
        if(n == MAX){
            i++;
            messages = realloc(messages, sizeof(char *)*(i+1));
            messages[i] = calloc(MAX, sizeof(char));
            n = 0;
        }
    }
    messages[i][n-1] = '\0';

    *size = i+1;
    return messages;
}

void free_messages(char **messages, int size){
    for(int i = 0; i < size; i++){
        free(messages[i]);
    }
    free(messages);
}


void chat(int socket_instance) {
    char **messages;
    int n; 
    int close = 0;
    char message[MAX];
    int n_messages;
    int size = 0;

    while(1){   
        // read how many messages must recieve 
        while(n_messages == 0){
            read(socket_instance, &n_messages, sizeof(n_messages)); 
        }
    
        // read and print the messages
        for(int i = 0; i < n_messages; i++){
            bzero(message, MAX);
            read(socket_instance, message, MAX); 
            printf("\nFrom Server : %s", message); 
        } 

        printf("\nEnter the message: "); 

        // gets the user message
        messages = read_message(&size);

        // check if the command to exit was entered 
        if(size == 1 && !strncmp("/quit", messages[0], 4))
            break;

        // send how messages will be sent to server side
        write(socket_instance, &size, sizeof(size)); 

        // send the messages
        for(int i = 0; i < size; i++)
            write(socket_instance, messages[i], MAX); 

        // free the buffer
        free_messages(messages, size);
        n_messages = 0;
    } 
} 
  

int main() 
{ 
    int socket_instance, connfd, len; 
    struct sockaddr_in serveraddress, cli; 
  
    // socket create and verification 
    socket_instance = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_instance == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&serveraddress, sizeof(serveraddress)); 
  
    // assign IP, PORT 
    serveraddress.sin_family = AF_INET; 
    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddress.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    int bind_ret;
    if ((bind_ret = bind(socket_instance, (SA*)&serveraddress, sizeof(serveraddress))) != 0) { 
        printf("socket bind failed...%d\n", bind_ret); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(socket_instance, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(socket_instance, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
 
    chat(connfd); 

    close(socket_instance); 
} 