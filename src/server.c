#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/types.h> 
#include <pthread.h>
#include <time.h>

#define PORT 8080 
#define SA struct sockaddr 

#define MAX 4096 
#define MAX_CLIENT_NAME 100 
#define MAX_SEND (MAX+MAX_CLIENT_NAME+2)
  

pthread_mutex_t mutex, mutex_confirm[20];
pthread_cond_t condition[20];
struct timespec max_wait;
int clients[20];
int clients_confirm[20];
pthread_t threads[20];
int n = 0;
int TIMEOUT = 2;

int send_ping(char *message, int current){
    char buffer[strlen(message)+1];
    strcpy(buffer, message);
    char *msg = strlen(strtok(buffer, ": ")) + buffer + 2;
    
    if(!strncmp("/ping", msg, 5)){
        if(send(current, "pong\n", strlen("pong\n"), 0) < 0) 
            printf("Sending failure \n");
        return 0;
    }
    return -1;
}

int close_connection(int socket_id){
    printf("Connection timeout, closing %d\n", socket_id);
    close(socket_id);
    return -1;

}

int confirm_send(int id, int len_checker){
    
    char message_received[MAX_SEND];

    pthread_mutex_lock(&(mutex_confirm[id]));

    printf("\n\n -------- Esperando %d... \n\n", id);
    max_wait.tv_sec = time(NULL) + TIMEOUT;
    max_wait.tv_nsec = 0;
    pthread_cond_timedwait(&(condition[id]), &(mutex_confirm[id]), &max_wait); 
    printf("\n\n -------- Liberou %d... \n\n", id);
    if(clients_confirm[id] != len_checker){
        printf("Error: client confirm fail.\n(client received) %d != (sent) %d\n\n", clients_confirm[id], len_checker);
        pthread_mutex_unlock(&(mutex_confirm[id]));
        return 0;
    }
    printf("\n--- recebendo confirm ----\n(client received) %d != (sent) %d\n\n", clients_confirm[id], len_checker);
    clients_confirm[id] = -1;
    pthread_mutex_unlock(&(mutex_confirm[id]));
    return 1;    
}

int sendtoall(char *message_to_send, int current){
    int i;
    pthread_mutex_lock(&mutex);
    int tries = 0, status = -1, confirmed = 0;

    
    for(i = 0; i < n; i++) {
        if((clients[i] != current) && (clients[i] != -1)) {
            printf("Enviando:<%s>\n", message_to_send);
            
            tries = 0;
            status = send(clients[i], message_to_send, strlen(message_to_send), 0);
            confirmed = confirm_send(i, strlen(message_to_send));

            while((status < 0 || !confirmed) && (tries < 4)) {
                printf("Sending failure to <%d>... Trying again (%d).\n", clients[i], tries);
                
                status = send(clients[i], message_to_send, strlen(message_to_send), 0);
                confirmed = confirm_send(i, strlen(message_to_send));
                tries++;
            }
            if(status < 0 || !confirmed){
                clients[i] = close_connection(clients[i]);
            } else {
                printf("Sending success to <%d>\n", clients[i]);
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void change_cofirm(char *value, int socket_instance){
    int id = -1;
    for(int i = 0; i < 20; i++)
        if(clients[i] == socket_instance)
            id = i;
    printf("\n ---------- verificando em %d value: %d\n\n", id, atoi(value));
    pthread_mutex_lock(&(mutex_confirm[id]));
    printf("\n ---------- recebendo em %d value: %d\n\n", id, atoi(value));
    clients_confirm[id] = atoi(value);
    pthread_cond_signal(&(condition[id]));
    pthread_mutex_unlock(&(mutex_confirm[id]));
}

int check_command(char *message, char socket_instance){
    char buffer[strlen(message)+1];
    strcpy(buffer, message);
    char *msg = strlen(strtok(buffer, ":")) + buffer + 1;
    
    if(!strncmp("/confirm", buffer, 7)){
        change_cofirm(msg, socket_instance);
        return 0;
    }
    return send_ping(message, socket_instance);
}

void *messager_receiver(void *socket_pointer){
    int socket_instance = *((int *)socket_pointer);
    char message_received[MAX_SEND];
    int len;

    
    while((len = recv(socket_instance, message_received, MAX_SEND,0)) > 0) {
        message_received[len] = '\0';
        if(check_command(message_received, socket_instance))
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
            printf("server acccept the client <%d>...\n", connfd); 
            
            pthread_mutex_lock(&mutex);
            
            clients[n]= connfd;
            n++;
            // creating a thread for each client 
            
            p_ret = pthread_create(&(threads[n]), NULL, (void *)messager_receiver, &connfd);
            pthread_cond_init(&(condition[n]), NULL);
            pthread_mutex_init(&(mutex_confirm[n]), NULL);   
        
            pthread_mutex_unlock(&mutex);
        }
    }

    for(int i = 0; i < n; i++){
        pthread_join(threads[n],NULL);
    }

    close(socket_instance); 
} 