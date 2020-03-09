//******************************************************************************
//******************************************************************************
//                    Dimitra Karatza
//                      AEM: 8828
//******************************************************************************
//******************************************************************************

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h> //error handling
#include <assert.h>   //timestamp to string convertion
#include <sys/socket.h>
#include <sys/types.h> //for server
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h> //for client
#include <signal.h>

#define MAX_LENGTH 256
#define MAX_MESSAGES 2000
#define PORT 2288
#define SA struct sockaddr
#define NUM_THREADS   3

//COLORS
#define BOLD_BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"

//************You need to change this when trying different AEMs***********************
//*************************************************************************************
#define RECEIVERS     3          //number of receivers
char AEM[4]= "8828";             //single AEM matched to this specific RasPi
char aem_list [3][5] = {"8808", "8794", "8884"}; //has to be the same in number with RECEIVERS
char address_list [3][11] = {"10.0.88.08", "10.0.87.94", "10.0.88.84"}; //has to contain the same AEMs as aem_list
//*************************************************************************************

//variables for keeping statistics to files
char file1[100]="my_messages.txt";
char file2[100]="all_messages.txt";
FILE *fp1, *fp2;
fd_set readfds; //set all socket descriptors

//initiallization of global variables
int duration = 7200;
char final_message[MAX_LENGTH];
pthread_t tid[NUM_THREADS];
pthread_mutex_t lock;
char *buffer[MAX_MESSAGES];       //circular buffer keeps list of node's messages
int msg_counter=0;                //general buffer index
int sent[RECEIVERS]= {0};         //indicates the last message to be sent

void alarmHandler(int sig)
{
  exit(0);
}

void print_buffer(){
  printf(BOLD_BLUE"\n\n          BUFFER          "RESET"\n");
  for (int i=0; i<msg_counter; i++){
    printf("%d.%s\n",i, buffer[i]);
  }
  printf("\n\n");
}

void write_msg_file(const char *file_msg, FILE *fp, char file[100]){
  //write message to the file
  fp = fopen(file, "a");
  fprintf(fp, "%s\n", file_msg);
  //close the file
  fclose(fp);
}

void write_tmstmp_file(time_t t, FILE *fp, char file[100]){
  //write time to the file
  fp = fopen(file, "a");
  fprintf(fp, "%ld\n", t);
  //close file
  fclose(fp);
}

void pop(const char *new_msg){

  int flag=0;

  //check for duplicate messages before inserting a message into the buffer
  for(int i=0;i<msg_counter;i++){
    if(strcmp(new_msg,buffer[i])==0){ //new_msg==buffer[i]
      flag=1;
      break;
    }
  }

  if(flag==0){

      if(msg_counter>=MAX_MESSAGES){

          //if circular buffer is full then roll the elements one position upwards
          for(int i=1;i<MAX_MESSAGES;i++){

            memset(buffer[i-1],0,strlen(buffer[i-1])); //clear the buffer[i-1] before copying
            strcpy(buffer[i-1],buffer[i]); //buffer[i-1]=buffer[i];
          }

          for(int i=0;i<RECEIVERS;i++){
            //update the array that contains the last messages sent to each receiver
            if(sent[i]!=0){
              sent[i]--; //pointer points to one element higher
            }else{
              sent[i]=0; //if the old first message is gone then send the new first message
            }            //if sent[i]=0 then it points to buffer[0], so we can't decrement the pointer more
          }

          msg_counter=MAX_MESSAGES-1; //buffer index points to the end of buffer

      }else{
          //if buffer has free space allocate the next new line
          buffer[msg_counter] = (char *)malloc(MAX_LENGTH);
      }

      //insert element into the buffer
      strcpy(buffer[msg_counter],  new_msg);
      msg_counter ++;

      //write message to the file "all_messages.txt"
      write_msg_file(new_msg,fp2,file2);

      //write the messages whose receiver is me in the file "my_messages.txt"
      int flag=1; //assume AEM is me
      for(int j=5;j<9;j++){
        if(buffer[msg_counter-1][j]!=AEM[j-5]){
          flag=0; //just one different letter means that AEM is not me
          break;  //I can exit the check procedure in just one different letter
        }
      }
      if(flag==1){
        write_msg_file(buffer[msg_counter-1],fp1,file1);
      }

  }
  print_buffer();
}

//function for Server
void chat_server(int sockfd)
{
    char buff[MAX_LENGTH];

    bzero(buff, MAX_LENGTH);

    while(recv(sockfd, buff, sizeof(buff),0)>0){
      // read messages that client sent and copy them in buffer

      pthread_mutex_lock(&lock);
      pop(buff);
      pthread_mutex_unlock(&lock);

      printf(YELLOW"\nJust got a message:"RESET" %s\n", buff);
      bzero(buff, MAX_LENGTH);
    }

}

//driver function for server
void driver_server(){

    char buff[MAX_LENGTH];

    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    socklen_t len;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /*if (sockfd == -1) {
        printf("server: socket creation failed...\n");
        exit(0);
    }else
        printf("Socket successfully created..\n"); */

    //Reuse the port: if client exits then allow them to reconnect
    if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int)) < 0)
      perror("setsockopt(SO_KEEPALIVE) failed");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        //printf("socket bind failed...\n");
        //exit(0);
    }
    //else
    //  printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, RECEIVERS)) != 0) {
        //printf("Listen failed...\n");
        //exit(0);
    }
    /*else{
        printf("Server listening..\n");
    }*/
    len =  sizeof(cli);


    for(;;){

      FD_ZERO(&readfds); //clear the socket set
      FD_SET(sockfd, &readfds); //add master socket to set

      if(FD_ISSET(sockfd, &readfds)){

        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA*)&cli, &len);

        /*if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        }else{
            printf("server accept the client...\n");
        }*/

        chat_server(connfd);

      }
    }
    // After chatting close the socket
    close(sockfd);
    sockfd=-1;
}


void *server(void *threadid){

  long tid;
  tid = (long)threadid;

  driver_server();

  pthread_exit(NULL);
}

void message_generator(){

  //initiallization
  int temp_aem, temp_msg_counter;   //reviever's AEM
  struct timeval cur_time;
  time_t tmstmp;
  char file5[100]= "time_generated_messages.txt";
  FILE *fp5;

  //random from aem_list to create a random receiver for the message
  temp_aem = rand()%3;

  //random from message list to create a random message
  char *messages[] = {"blue","yellow", "purple", "green", "pink"};
  temp_msg_counter = rand()%5;

  //get timestamp
  gettimeofday(&cur_time, NULL);
  tmstmp = (time_t) cur_time.tv_sec;

  //write timestamp in file5
  write_tmstmp_file(tmstmp, fp5, file5);

  //convert long timestamp to string
  const int m = snprintf(NULL, 0, "%lu", tmstmp);
  assert(m > 0);
  char str_timestmp[m+1];
  int c = snprintf(str_timestmp, m+1, "%lu", tmstmp);
  assert(str_timestmp[m] == '\0');
  assert(c == m);

  //create the appropriate message format
  strcpy(final_message, "8828");
  strcat(final_message, "_");
  strcat(final_message, aem_list[temp_aem]);
  strcat(final_message, "_");
  strcat(final_message, str_timestmp);
  strcat(final_message, "_");
  strcat(final_message, messages[temp_msg_counter]);

  // final_message;
  printf(MAGENTA"\nJust created a message:"RESET" %s\n", final_message);

  //fill buffer with newly created message
  pthread_mutex_lock(&lock);
  pop(final_message);
  pthread_mutex_unlock(&lock);
}

void chat_client(int sockfd, int temp_id){

  int flag; //flag indicates if the final receiver is my AEM, so no need for sending
  useconds_t r=((rand()%5)+3);

  for(int i=sent[temp_id]; i<msg_counter; i++){

    flag=1; //assume AEM is me

    for(int j=5;j<9;j++){
      if(buffer[i][j]!=AEM[j-5]){
        flag=0; //just one different letter means that AEM is not me
        break;   //I can exit the check procedure in just one different letter
      }
    }

    //if receiver is not me, then send this message
    if(flag==0){
      send(sockfd, buffer[i], strlen(buffer[i])+1, 0);
      printf(CYAN"\nJust sent a message:"RESET" %s\n", buffer[i]);
      sent[temp_id]=msg_counter;
      usleep((useconds_t)(0.2 * 100000));    //control sending speed
    }
  }

  usleep(r * 1000000); //random sleep time: 0.3<r<0.7
}

//driver function for client
void driver_client()
{
    //initiallization of variables
    int temp_id=-1;
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    char file3[100]= "connection_timestamps.txt";
    char file4[100]= "connection_duration.txt";
    FILE *fp3, *fp4;
    time_t start_timestamp, end_timestamp;
    time_t prev_timestamp=0;

    for(;;){

      do{

        // socket create and varification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        /*if (sockfd == -1) {
            printf("client: socket creation failed...\n");
            exit(0);
        }else
          printf("Socket successfully created..\n"); */

        bzero(&servaddr, sizeof(servaddr));

        temp_id++;
        if(temp_id>=RECEIVERS){
          temp_id=0;
        }
        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(address_list[temp_id]); //pick an address bettween address_list[0:len]
        servaddr.sin_port = htons(PORT);

      }while(connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0);

      //Write to file3 the difference in time between each connection
      start_timestamp = time(NULL);
      write_tmstmp_file(start_timestamp - prev_timestamp, fp3, file3);
      //save current timestamp to prev_timestamp for the next connection
      prev_timestamp=start_timestamp;

      // function for chat
      chat_client(sockfd, temp_id);

      end_timestamp = time(NULL);
      //Write to file4 the duration of each connection
      write_tmstmp_file(end_timestamp - start_timestamp, fp4, file4);

      // close the socket
      close(sockfd);
      sockfd=-1;
    }
}


void *client(void *threadid)
{

  long tid;
  tid = (long)threadid;

  driver_client();
  pthread_exit(NULL);
}

void *message(void *threadid){
  srand(getpid());
  long tid;
  tid = (long)threadid;

  for(;;){
    message_generator();
    sleep((rand()%240)+60);   //produce messages every 1-5 minutes
  }

  pthread_exit(NULL);
}

void intro()
{
  printf("...............................................\n"); //47
  printf("                                               \n");
  printf("                                               \n");
  printf("                                               \n");
  printf(GREEN"             *******************            \n");
  printf("                                               \n");
  printf(RED"                 RaspberryPi                   \n");
  printf("                                               \n");
  printf(GREEN"             *******************            \n");  printf("                                               \n");
  printf("                                               \n");
  printf("                                               \n");
  printf("                                               \n");
  printf(RESET"        Produced by Dimitra Karatza 8828       \n");
  printf("                                               \n");
  printf("                                               \n");
  printf("                                               \n");
  printf("                                               \n");
  printf("             ~Start of program~                \n");
  printf("                                               \n");
  printf("                                               \n");
  printf("                                               \n");
  printf(RESET"...............................................\n");
  printf("                                               \n");
  printf("                                               \n");

}

int main(int argc, char **argv){

  signal(SIGALRM, alarmHandler);
  alarm(duration); //set a 2 hour alarm

  intro();

  srand(time(NULL));

  //initiallization of variables
  pthread_t threads[NUM_THREADS];
  int rc[NUM_THREADS];
  long t;

  for(t=0; t<NUM_THREADS; t++){

    //initiallize thread
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    //create each thread according to value of t
    if(t==0){
      rc[t] = pthread_create(&threads[t], NULL, server, (void *)t);
    }else if(t==1){
      rc[t] = pthread_create(&threads[t], NULL, client, (void *)t);
    }else{
      rc[t] = pthread_create(&threads[t], NULL, message, (void *)t);
    }

    //error handling
    if (rc[t]){
      printf("ERROR; return code from pthread_create() is %d\n", rc[t]);
      exit(-1);
    }
  }

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  pthread_join(tid[2], NULL);
  pthread_mutex_destroy(&lock);

  // Last thing that main() should do
  pthread_exit(NULL);

  return 0;
}
