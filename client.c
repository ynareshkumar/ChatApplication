#include <stdio.h>
#include <stdlib.h>
#include <errno.h>		// defines perror(), herror() 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "sbcpformat.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>		// gethostbyname()

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>		// for select() system call only
#include <sys/timeb.h>		// time(), ftime() functions

#define DEFAULT_PORT_NUMBER 5000
#define BUFFER_LENGTH 1024
#define DEFAULT_HOST "127.0.0.1"


//broadcastmessage is the message constructed by client to be sent for server. receivingmessage is the mesage received from server.
struct sbcpmessage broadcastmessage,receivingmessage;
//Construct the message to be sent to server
void formmessage(char user[16],char message[512]);

int main(int argc,char *argv[])
{
  int port,sd,bytestoread,numofbytes;//sd is the socket descriptor of the client,numofbytes is the number of bytes read.
  int fdmax,i,j,k,selectresult,nread;//fdmax is the maximum number of connected clients,i,j,k are temp variables 
//host is the ip address of server,buffer is a pointer to the string buffer,sendbuf is buffer which stores the value to be sent to server
  char *host,*buffer,sendbuf[BUFFER_LENGTH],receivebuf[BUFFER_LENGTH];
  fd_set master;//master socket descriptor
  fd_set read_fds;//temporary socket descriptor

  struct sockaddr_in serveraddress;//Contains server details like port and IP address
  struct hostent *hp;//gethostbyname()
  struct clientattributes client;//contains client details like username and socket id
  //struct timeval timeout;
 
//Get command line argumentvalues for username, server IP and port
  if(argc == 4)
    {
      strcpy(client.username,argv[1]);
      host = argv[2];
      port = atoi(argv[3]);
    }
  else//Force exit
    {
      printf("\n The number of arguments entered is %d",argc-1);
      printf("\n Please enter the 3 arguments in username hostip port format \n");
      exit(1); 
    }

   //Create socket
   sd = socket(AF_INET,SOCK_STREAM,0);

  //Check for failure for creating socket
  if(sd == -1)
    {
       printf("\n Unable to create socket\n");
       exit(1);
    }

//set as 0 initially
  memset(&serveraddress,0,sizeof(serveraddress));
  serveraddress.sin_family = AF_INET;
  serveraddress.sin_port = htons(port);
  hp = gethostbyname(host);
  if(hp == NULL)
  {
    printf("\n Unable to get host address\n");
    exit(1);
  }
//Copy host address to server address
  bcopy(hp->h_addr,(char *)&serveraddress.sin_addr,hp->h_length);

//Establish connection to server
 if(connect(sd,(struct sockaddr *)&serveraddress,sizeof(serveraddress)) == -1)
  {
    printf("\nCannot connect to server\n");
    exit(1);
  }
  
 
  printf("\n Connected to server whose address is %s",hp->h_name);
  printf("\n Type JOIN to join the group chat and then start chatting \n");
  printf("\n Press Esc and Enter or Ctrl+C to quit from the chat anytime \n");

//Set the master and temporary descriptors to ).
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(sd,&master);  
  FD_SET(STDIN_FILENO,&master);
  fdmax = sd;
  
//Infinite loop
   for(;;)
    {
     read_fds = master;//copy master to temporary

     //Wait for a input read or write from server
     selectresult = select(fdmax+1, &read_fds, NULL, NULL, NULL);
     if(selectresult == -1)
      {
        perror("select");
        exit(4);
      }

     if(selectresult == 0)
     {
	printf("\nIdle");
        break;
     }
	      //Is it input read from client?
              if(FD_ISSET(STDIN_FILENO,&read_fds)){                
                
		bzero(sendbuf,BUFFER_LENGTH);
                nread = read(STDIN_FILENO, sendbuf, sizeof(sendbuf));  //Read the input entered in the client
                if(sendbuf[0] == 27)//If the data entered to Esc character, exit from client
                {
                   printf("\n Exiting...\n");
		   close(sd);
                   exit(1);
                }
                else{
                   //printf("\n Sendbuf value is %s",sendbuf);
                }
		 
		//Form message to be sent for server      
		formmessage(client.username,sendbuf);   
		//Write to the server
	  	write(sd,(void *) &broadcastmessage,sizeof(broadcastmessage));
	    	  	
          }//End for else if ISSET
	//Is it write event from server?
         if(FD_ISSET(sd,&read_fds))
         {
          bzero(receivebuf,BUFFER_LENGTH);
          buffer = receivebuf;
  	  bytestoread = BUFFER_LENGTH;
	  //Read the message from server
          numofbytes = read(sd,(struct sbcpmessage *) &receivingmessage,sizeof(receivingmessage));

         // printf("Number of bytes to read = %d", numofbytes);	
	      //If number of bytes read is less than or equal to zero, the server has disconnected. Force exit the client.			
              if(numofbytes <= 0)
               {
		printf("\n Server Disconnected. So terminating\n");
		exit(1);
	      }   	
		//If receiving message type is NAK, the username already exists. So force exit client.
		if(receivingmessage.type == '5')
		{
		printf("\n%s\n",receivingmessage.attributes[0].payload);
		exit(1);
		}
		//If receiving message type is ACK, print the payload messages which shows the clients count and active clients.
		if(receivingmessage.type == '7')
		{
			for(j=0;j<receivingmessage.length;j++)
				printf("%s",receivingmessage.attributes[j].payload);	
		}
                //If receiving message is Online/Offline, print the respective status message.
		if(receivingmessage.type == '6' || receivingmessage.type == '8' || receivingmessage.type == '3'){
			printf("\n%s\n",receivingmessage.attributes[0].payload); 
			
		 }
		//If receiving message is FWD, print the message and the username who sent the message.
		/*else if(receivingmessage.type == '3')
		 {
		        printf("\n%s says:%s\n",receivingmessage.attributes[0].payload,receivingmessage.attributes[1].payload); 
		 }   */  

          }
  }//End for infinite loop
 close(sd);

return 0;
}

//Construct the message to be sent to server
void formmessage(char user[16],char message[512])
{
  int i;
  char clientmessage[528];
  broadcastmessage.vrsn = 3;
  broadcastmessage.length = 1;//1 payload only
 
  if(strcmp(message,"JOIN\n") == 0)
  {
   broadcastmessage.type = '2';//JOIN message type
   broadcastmessage.attributes[0].type = 2;//username field
   broadcastmessage.attributes[0].payloadlength = strlen(user);
   strcpy(broadcastmessage.attributes[0].payload, user);
       
  }
  else
  {
   broadcastmessage.type = '4';//SEND message type

   broadcastmessage.attributes[0].type = 4;//Actual chat message
   strcpy(clientmessage,user);
   strcat(clientmessage," says:");
   strcat(clientmessage,message);
   broadcastmessage.attributes[0].payloadlength = strlen(clientmessage);
   strcpy(broadcastmessage.attributes[0].payload , clientmessage); 
   bzero(clientmessage,528);
  }
}
