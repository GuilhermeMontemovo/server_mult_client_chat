#include <netdb.h> 
#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <pthread.h>

#define MAX 4096 
#define MAX_CLIENT_NAME 100 
#define MAX_SEND (MAX+MAX_CLIENT_NAME+2)
#define PORT 8080 
#define SA struct sockaddr 
#define CONNECT_CMD "/connect"

int check_commands(char *message){
    return !strncmp("/quit", message, 4);
}      
    

void chat(void *socket_pointer, char *client_name){  
    int socket_instance = *((int *)socket_pointer);
    char message_read[MAX];
    char message_to_send[MAX_SEND];
    

    
    while (fgets(message_read,MAX,stdin) > 0) { 
        strcpy(message_to_send,client_name);
        strcat(message_to_send,": ");

        printf("\nEnter the message: "); 

        
        // check if the command to exit was entered 
        if(check_commands(message_read))
            break;

        strcat(message_to_send,message_read);

        if(write(socket_instance, message_to_send, strlen(message_read)) < 0)
            printf("\nError: message not sent\n");
        
    } 
} 


void *messager_receiver(void *socket_pointer){
    int socket_instance = *((int *)socket_pointer);
    char message_received[MAX_SEND];
    int len;

    while((len = recv(socket_instance, message_received, MAX_SEND,0)) > 0) {
        message_received[len] = '\0';
        fputs(message_received,stdout);
    }
}


int main(int argc, char *argv[]) { 
    pthread_t receiver_token;
    int socket_instance, connfd; 
    struct sockaddr_in serveraddress; 
    
    if(argc < 2) {
        printf("Usage: ./client <client_name>");
        return -1;
    }

    char client_name[MAX_CLIENT_NAME];
    strcpy(client_name, argv[1]);


    // socket create and varification 
    socket_instance = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_instance == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&serveraddress, sizeof(serveraddress)); 

    char ip_address[18];
    printf("Please, enter \'IP Address\' to bind\n");
    scanf("%[^\n]%*c", ip_address);
    printf("Type \'/connect\' to stabilsh connection with the server on %s\n", ip_address);

    // assign IP, PORT 
    serveraddress.sin_family = AF_INET; 
    serveraddress.sin_addr.s_addr = inet_addr(ip_address); 
    serveraddress.sin_port = htons(PORT); 
    
    char connect_cmd[10];
    static const char *compare_cmd = CONNECT_CMD;
    
    while(fgets(connect_cmd,9,stdin) > 0 && strcmp(connect_cmd, compare_cmd)){
        printf("Invalid command, please type \'/connect\' to stabilsh connection with the server\n");
    }


    // connect the client socket to server socket 
    if (connect(socket_instance, (SA*)&serveraddress, sizeof(serveraddress)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server...\n"); 
        
    //creating thread to always wait for a message
    pthread_create(&receiver_token,NULL,(void *)messager_receiver,&socket_instance);

    chat(&socket_instance, client_name); 

    //close thread
    pthread_join(receiver_token,NULL);

    close(socket_instance); 
} 