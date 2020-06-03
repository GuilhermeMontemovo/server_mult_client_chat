#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <time.h>

#include <netdb.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#include <pthread.h>
#include <signal.h>
#include <sys/types.h> 

#define EXIT_CMD "/quit"

#define PORT 8080 
#define SA struct sockaddr 

#define MAX 4096 // max size of a message
#define MAX_CLIENT_NAME 100  // max size of a client name
#define MAX_SEND (MAX+MAX_CLIENT_NAME+2)
  
volatile sig_atomic_t keepRunning; 

pthread_mutex_t mutex, mutex_confirm[20]; // mutex: used to send messages; mutex_confirm: used to accces message confirmed value of a client 
pthread_cond_t condition[20]; // condition: used with <mutex_confirm> to tell when a confirm message is ready
pthread_t threads[20]; // thread ids for each client connected
struct timespec max_wait; 
int TIMEOUT = 4; // max time to wait a confirm message

int clients[20]; // connection id for each client
int clients_confirm[20]; // confirm message for each client

int n = 0, socket_master; // n: number of clients connected counter; socket_master: socket id


int send_ping(char *message, int current){
    /*
        check if a message is equal to "/ping" and, if true, send "pong" back to the client

        message: string to check equals "/ping"
        current: client id

    */
    char buffer[strlen(message)+1];
    strcpy(buffer, message);
    char *msg = strlen(strtok(buffer, ": ")) + buffer + 2; // removing client name
    
    if(!strncmp("/ping", msg, 5)){
        if(send(current, "pong\n", strlen("pong\n"), 0) < 0) 
            printf("Sending pong to %d failure \n", current);
        return 0;
    }
    return -1;
}

int close_connection(int id){
    /*
        terminate thread and connection with a client

        id: client id
    */
    printf("Connection timeout, closing %d: %d\n", id, clients[id]);
    pthread_detach(threads[id]);
    close(clients[id]);
    return -1;
}

int confirm_send(int id, int len_checker){
    /*
        confirm if a client recived the same amout of bytes that was send

        id: client id
        len_chekcer: amout of bytes sent
    */
    char message_received[MAX_SEND];

    pthread_mutex_lock(&(mutex_confirm[id]));

    max_wait.tv_sec = time(NULL) + TIMEOUT;
    max_wait.tv_nsec = 0;
    // await client answer confirm or TIMEOUT pass.
    pthread_cond_timedwait(&(condition[id]), &(mutex_confirm[id]), &max_wait); 

    if(clients_confirm[id] != len_checker){
        printf("Error: client confirm fail.\n(client received) %d != (sent) %d\n\n", clients_confirm[id], len_checker);
        pthread_mutex_unlock(&(mutex_confirm[id]));
        return 0;
    }
    
    clients_confirm[id] = -1;
    pthread_mutex_unlock(&(mutex_confirm[id]));
    return 1;    
}

int sendtoall(char *message_to_send, int current){
    /*
        send a message to all clients connected excepted the sender

        message_to_send: message to be send
        current: client sender
    */
   
    int i;
    pthread_mutex_lock(&mutex);
    int tries = 0, status = -1, confirmed = 0;
    int err = 0, error;
    socklen_t size = sizeof (err);

    for(i = 0; i < n; i++) {
        if((clients[i] != current) && (clients[i] != -1)) { // skip sender and check if connection dind't close 
            
            tries = 0;
            
            error = getsockopt (clients[i], SOL_SOCKET, SO_ERROR, &err, &size);
            
            if(!error){ // check if connection is open
                status = send(clients[i], message_to_send, strlen(message_to_send), 0);
                confirmed = confirm_send(i, strlen(message_to_send));
            }
            // if can't send or client can't confirm, try more 4 times
            while((status < 0 || !confirmed) && (tries < 4)) {
                printf("Sending failure to <%d>... Trying again (%d).\n", clients[i], tries);
                
                error = getsockopt (clients[i], SOL_SOCKET, SO_ERROR, &err, &size);
                if(!error){
                    status = send(clients[i], message_to_send, strlen(message_to_send), 0);               
                    confirmed = confirm_send(i, strlen(message_to_send));
                }
                tries++;
            }
            if(status < 0 || !confirmed){
                // if client connection is broken, close connection
                clients[i] = close_connection(i);
            } else {
                printf("Sending success to <%d>\n", clients[i]);
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void change_cofirm(char *value, int socket_instance){
    /*
        change message confirm value of a client
        
        value: int as string
        socket: connection id of this client
    */
    int id = -1;
    for(int i = 0; i < 20; i++) // get de id by the connection id
        if(clients[i] == socket_instance)
            id = i;
    
    pthread_mutex_lock(&(mutex_confirm[id]));
    
    clients_confirm[id] = atoi(value);
    pthread_cond_signal(&(condition[id])); // alerting that confirm was updated
    pthread_mutex_unlock(&(mutex_confirm[id]));
}

int check_command(char *message, char socket_instance){
    /*
        check if a messagem is a internal command, such as:
        /ping
        /confirm (confirme message)

        message: message to check
        socket_instance: current connection id
    */
    char buffer[strlen(message)+1];
    strcpy(buffer, message);
    char *msg = strlen(strtok(buffer, ":")) + buffer + 1; // removing cient name
    
    if(!strncmp("/confirm", buffer, 7)){
        change_cofirm(msg, socket_instance);
        return 0;
    }
    return send_ping(message, socket_instance);
}

void *messager_receiver(void *socket_pointer){
    /*
        recieve all incoming messages from a client

        socket_pointer: cleint to listen
    */
    int socket_instance = *((int *)socket_pointer);
    char message_received[MAX_SEND];
    int len;

    
    while((len = recv(socket_instance, message_received, MAX_SEND,0)) > 0) {
        message_received[len] = '\0';
        if(check_command(message_received, socket_instance))
            sendtoall(message_received, socket_instance);
    }
    pthread_exit(NULL);
}

void intHandler(int signum) {
    /*
        Signal Handler 
    */
    int i;
    keepRunning = 0;
    
    for(i = 0; i < n; i++) //close threads
        if(threads[i] && pthread_kill(threads[i], 0) == 0)
            pthread_detach(threads[i]);
    
    close(socket_master);  // close socket
    exit(0);
}

int main(){ 
    int connfd, len; 
    struct sockaddr_in serveraddress, cli; 
    
    signal(SIGINT, &intHandler);

    // socket create and verification 
    socket_master = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_master == -1) { 
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
    if ((bind_ret = bind(socket_master, (SA*)&serveraddress, sizeof(serveraddress))) != 0) { 
        printf("socket bind failed...%d\n", bind_ret); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(socket_master, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    
    len = sizeof(cli); 
    int p_ret = 0;
    
    char connect_cmd[10];
    static const char *compare_cmd = EXIT_CMD;

    
    keepRunning = 1;
    while(keepRunning){
        if((connfd = accept(socket_master, (SA*)&cli, &len)) < 0)
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
} 