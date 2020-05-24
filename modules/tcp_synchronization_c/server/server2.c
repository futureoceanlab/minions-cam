//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux  
#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include <time.h>
#include <wiringPi.h>
#include <signal.h>
     
#define PORT 8080  
#define BILLION 1000000000LL
#define TRIG_PIN 24
#define NUM_AVG 100

void handler(int signo)
{
    digitalWrite(TRIG_PIN, HIGH);
    for (int i =0; i < 1000; i++) {}
    //printf("Server: tick\n");
    digitalWrite(TRIG_PIN, LOW);
}


long long as_nsec(struct timespec *T)
{
    return ((long long) T->tv_sec)*BILLION + (long long) T->tv_nsec;
}


long long bytes_to_nsec(char *buffer)
{
    time_t sec = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    int nsec = (buffer[7] << 24) | (buffer[6] << 16) | (buffer[5] << 8) | buffer[4];
    return ((long long) sec) * BILLION + (long long) nsec;
}


void as_timespec(long long t, struct timespec *T)
{
    T->tv_sec = (long) (t/BILLION);
    T->tv_nsec = (long) (t % BILLION);
    return;
}     


int main(int argc , char *argv[])   
{   
    wiringPiSetup();
    pinMode(TRIG_PIN, OUTPUT);

    int opt = 1;   
    int master_socket , addrlen , new_socket , client_socket[30] ,  
          max_clients = 2 , activity, i , valread , sd;   
    int max_sd;   
    struct sockaddr_in address;   
         
    char buffer[1025];  //data buffer of 1K  
         
    //set of socket descriptors  
    fd_set readfds;   
         
    //a message  
    char *message = "ECHO Daemon v1.0 \r\n";   
     
    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }   
         
    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   
         
    //bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT);   
         
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    //accept the incoming connection  
    addrlen = sizeof(address);   
    puts("Waiting for connections ...");   
    struct timespec T_start = {.tv_sec=0, .tv_nsec=0};
    struct timespec T2, T3;
    uint8_t sync[2] = {0};    

    while(sync[0] != 2 || sync[1] != 2)   
    {   
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(master_socket, &readfds);   
        max_sd = master_socket;   
             
        //add child sockets to set  
        for ( i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }   
        // tk     
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(master_socket, &readfds))   
        {   
            if ((new_socket = accept(master_socket,  
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n", 
                new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
           
            /*//send new connection greeting message  
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )   
            {   
                perror("send");   
            }   
                 
            puts("Welcome message sent successfully");   */
                 
            //add new socket to array of sockets  
            for (i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);   
                         
                    break;   
                }   
            }   
        }   
        // tk     
        //else its some IO operation on some other socket 
        for (i = 0; i < max_clients; i++)   
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = read( sd , buffer, 1024)) == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&address, (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n",  
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    client_socket[i] = 0;   
                }   
                     
                // handle synchronization 
                else 
                {   
                    clock_gettime(CLOCK_REALTIME, &T2);
                    long long rec_T = bytes_to_nsec(buffer);
//                    printf("%lld\n", rec_T);
                    if (rec_T > 1) 
                    {
                        usleep(1000);
                        clock_gettime(CLOCK_REALTIME, &T3);
                        
                        char timeData[16] = {0};
                        char *T2_arr = (char *) &(T2.tv_sec);
                        char *T3_arr = (char *) &(T3.tv_sec);
                        for (int i = 0; i < 8; i++) {
                            timeData[i] = T2_arr[i];
                            timeData[8+i] = T3_arr[i];
                        }
                        send(sd, timeData, 16, 0);   
                    }
                    else if (rec_T == 0)
                    {
                        sync[i] = 1;
                        if (sync[0] == 1 && sync[1] == 1)
                        {
                            if (T_start.tv_sec == 0 && T_start.tv_nsec == 0)
                            {
                                printf("only once");
                                long long T2n = as_nsec(&T2);
                                T2n += 2*BILLION;
                                as_timespec(T2n, &T_start);
                            }
                        }
                        send(sd, (char *) &T_start, 8, 0);
                    }
                    else if (rec_T == 1)
                    {
                        sync[i] = 2;
                        char timeData[8] = {0};
                        timeData[4] = 1;
                        send(sd, timeData, 8, 0); 
                    }
                }
            }   
        }   
    }   
     
    timer_t t_id;
    struct itimerspec tim_spec = {.it_interval= {.tv_sec=1,.tv_nsec=0},
                    .it_value = T_start};

    struct sigaction act;
    sigset_t set;

    sigemptyset( &set );
    sigaddset( &set, SIGALRM );

    act.sa_flags = 0;
    act.sa_mask = set;
    act.sa_handler = &handler;

    sigaction( SIGALRM, &act, NULL );

    if (timer_create(CLOCK_REALTIME, NULL, &t_id))
        perror("timer_create");

    if (timer_settime(t_id, TIMER_ABSTIME, &tim_spec, NULL))
        perror("timer_settime");
    while(1);
    return 0;   
}
