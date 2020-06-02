#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/types.h> 
#include <pthread.h>

#define PORT 8080 
#define SA struct sockaddr 

#define MAX 4096 
#define MAX_CLIENT_NAME 100 
#define MAX_SEND (MAX+MAX_CLIENT_NAME+2)
  

pthread_mutex_t mutex;
int clients[20];
pthread_t threads[20];
int n = 0;

int send_ping(char *message, int current){
    //  TODO fix ping send delay
    char *msg = strlen(strtok(message, ": ")) + message + 2;
    
    if(!strncmp("/ping", msg, 5)){
        if(send(current, "pong", strlen("pong"), 0) < 0) 
            printf("Sending failure \n");        
        return 0;
    }
    return -1;
}

void sendtoall(char *message_to_send, int current){
    int i;
    pthread_mutex_lock(&mutex);

    if(send_ping(message_to_send, current))
        for(i = 0; i < n; i++) {
            if(clients[i] != current) {
                // TODO try 5 times until close conection
                if(send(clients[i], message_to_send, strlen(message_to_send), 0) < 0) {
                    printf("Sending failure \n");
                    continue;
                }
            }
        }
    pthread_mutex_unlock(&mutex);
}


void *messager_receiver(void *socket_pointer){
    int socket_instance = *((int *)socket_pointer);
    char message_received[MAX_SEND];
    int len;

    
    while((len = recv(socket_instance, message_received, MAX_SEND,0)) > 0) {
        message_received[len] = '\0';
        sendtoall(message_received, socket_instance);
    }

}
  

int main(){ 
    int socket_instance, connfd, len; 
    struct sockaddr_in serveraddress, cli; 
    // pthread_t receiver_token;
  
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
    serveraddress.sin_addr.s_addr = inet_addr("127.0.0.1"); 
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
  
    int p_ret = 0;
    while(1){
        if((connfd = accept(socket_instance, (SA*)&cli, &len)) < 0)
            printf("accept failed  \n");
        else{
            printf("server acccept the client...\n"); 
            
            pthread_mutex_lock(&mutex);
            
            clients[n]= connfd;
            n++;
            // creating a thread for each client 
            
            p_ret = pthread_create(&(threads[n]), NULL, (void *)messager_receiver, &connfd);
            
            pthread_mutex_unlock(&mutex);
        }
    }

    for(int i = 0; i < n; i++){
        pthread_join(threads[n],NULL);
    }

    close(socket_instance); 
} 