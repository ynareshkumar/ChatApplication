#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sbcpformat.h"

#define DEFAULT_PORT_NUMBER 5000
#define DEFAULT_QUEUE_LENGTH 5
#define BUFFER_LENGTH 1024

  int port,sd,listenresult,newfd,bytestoread,numofbytes;  //sd is socket descriptor,numofbytes is number of bytes read from client
  int MAX_CLIENTS;//Maximum number of clients
  int joinedornot[100];//An array storing whether a client has Joined the chat by explicitly sendimg JOIN command. It takes 0 or 1.
  int validusername=1;//A temporary variable stating whether the username is already in use or not
  int fdmax,selectresult,i,j,k,p,temp;//fdmax is maximum number of connected sockets,i,j,k,p,temp are temporary variables for loop
  char *buffer,buf[BUFFER_LENGTH];//buffer is a pointer to the string buffer,buf is the actual string buffer for read/write
  char clientnames[512];//Used to store the server message which is to be sent to the clients
  char *ipaddress;//IP address of the server from command line
  struct sockaddr_in serveraddress;//It is a struct object containing server details like port,address
  struct sbcpmessage message,servermessage;//Messages to construct a SBCP message to be sent to client
  fd_set master;//master descriptor containing all descriptors
  fd_set read_fds;//temporary descriptor
  int yes = 1;//Used to state if socket is reusable
  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;//length of the client address
  int numofclients = 0;//Number of active clients in the current session

void closeclientconnection(struct clientattributes clientdetails[]);//Detect exit of client, close connection and notify other users
void checkforvalidusername(struct clientattributes clientdetails[]);//Check if username is already in use.If yes, send message to client.
void sendconnectedclients(struct clientattributes clientdetails[]);//When a client successfully joins, send the list of active clients.

int main(int argc,char *argv[])
{
  //Initialize all the clients to be not joined initially
  for(p=0;p<100;p++)
   joinedornot[p] = 0;
 
  //Get command line arguments for server IP,port and maximum number of clients. If wrongly entered, exit.
  if(argc == 4)
	{    
     ipaddress = argv[1]; 	
     port = atoi(argv[2]);
     MAX_CLIENTS = atoi(argv[3]);
	}
  else//Force exit
      {
      printf("\n The number of arguments entered is %d",argc-1);
      printf("\n Please enter the 2 arguments in port Maxclients format \n");
      exit(1); 
      }
 	
    //Store details of all connected clients like username and socket id
    struct clientattributes clientdetails[MAX_CLIENTS];
 //Create a socket 
  sd = socket(AF_INET,SOCK_STREAM,0);

//Check for any error in socket creation
  if(sd == -1)
    {
       printf("\n Unable to create socket");
       exit(1);
    }

  //Reuse the socket
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  
  //Initialize value to 0
  memset(&serveraddress,0,sizeof(serveraddress));
  serveraddress.sin_family = AF_INET;
  serveraddress.sin_port = htons(port);//host to network order conversion
  serveraddress.sin_addr.s_addr = inet_addr(ipaddress);
  
  //Assign the socket to a address
  if(bind(sd,(struct sockaddr *)&serveraddress,sizeof(serveraddress)) == -1)
  {
    printf("\n Unable to bind socket");
    exit(1);
  }
 
 //Listen for connection
  listenresult = listen(sd,MAX_CLIENTS);

//Check for error in listening
  if(listenresult == -1)
   {
     printf("\n Unable to listen.");
     exit(1);
   }  

   //Initialize the master and temporary file descriptors to ZERO
   FD_ZERO(&master);
   FD_ZERO(&read_fds);
   fdmax = sd;
   FD_SET(sd, &master);

// Infinite loop
    for(;;) {
        bcopy(&master,&read_fds,sizeof(master)); //copy the master to temporary
        
        //Wait for an event like connect or data operation
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        //Run through the existing connections looking for the matched event
        for(i = 0; i <= fdmax; i++) {
	//Each time the username field is initialized to be valid(1). This is will be changed in the loop if the username is found to be invalid.
	 validusername = 1;
         //printf(" fdmax is %d ; monitoring of fd %d is %s\n", fdmax, i, (FD_ISSET(i, &read_fds))?"true":"false");
            if (FD_ISSET(i, &read_fds)) { // We got some event
                if (i == sd) {
                    // Handle new connections
                    addrlen = sizeof remoteaddr;
		    //Accept new connection from client
                    newfd = accept(sd,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
		    printf("Connection established for the client\n");
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // Check for the maximum value
                            fdmax = newfd;
                        }                        
                    }
                } else {
                    // handle data from a client
                             bzero(buf,BUFFER_LENGTH);
			     buffer = buf;
			     bytestoread = BUFFER_LENGTH;
			     //printf("\n Entering data read part for i = %d \n",i);
                             //Read data from client and store it in message
			     numofbytes = read(i,(struct sbcpmessage *) &message,sizeof(message));
                            // printf("Number of bytes to read = %d", numofbytes);
			    
			     //Check if the number of bytes is less than or equal to 0. If it is, then the client has closed the connection.				
                             if(numofbytes <= 0)
			      {
 				closeclientconnection(clientdetails);	
				//Continue to the next iteration as the connection is closed.			
				continue;
			      }          

			     //Check for valid username. Do it for only message type 2 as it corresponds to JOIN message from client.
			      if(message.type == '2')
                               {	//Check for maximum clients reach condition. If it is, then send message to client.
				if(numofclients == MAX_CLIENTS)
				{
				   strcpy(clientnames,"Maximum clients in the chat session reached. Please try later\n");
		  		   printf("\n Client Name Message is: %s",clientnames);
		 	     	   servermessage.vrsn = 3;
			     	   servermessage.type = '5';//NAK message type
			     	   servermessage.length = 1;
		                   servermessage.attributes[0].type = 1;//Reason for failure field
	     			   servermessage.attributes[0].payloadlength = strlen(clientnames);
	     			   strcpy(servermessage.attributes[0].payload,clientnames);
	     			   write(i, (void *) &servermessage, sizeof(servermessage));
				   FD_CLR(i,&master);
                		   bzero(clientnames,512); 
				   break;
				}
			        checkforvalidusername(clientdetails);						
			      }                   
			     // printf("\n Message type %c \n",message.type);
			     //If it is a valid username, send the list of active clients to that client and notify the other users.
                             if(message.type == '2' && validusername == 1)
                               {	
				 sendconnectedclients(clientdetails);	                                  
                               }	

			    //Broadcast the incoming message to all other clients
			    else if(validusername == 1 && joinedornot[i] == 1)
			    {                            
    		
			     message.type = '3';
			     for(k = 0; k <= fdmax; k++) {
                            // send to everyone!
		                    if (FD_ISSET(k, &master)) {
		                        // except the listener and ourselves
		                        if (k != sd && k != i && joinedornot[k] == 1) {
						printf("\n Broadcasting..\n");
		                            if (write(k, (void *) &message, sizeof(message)) == -1) {
		                                perror("send");
		                            }
		                        }
		                    }
		                 }
                             
      			   }

                } // End handle data from client
            } // End the Event detection condition ISSET
        } // End loop through each file descriptors
    } // End for infinite loop
    
    return 0;
}

void closeclientconnection(struct clientattributes clientdetails[])
{
	printf("\n Connection closed in client \n");
	//As connection is closed, remove from master file descriptor
	FD_CLR(i,&master);
	//Find the position of the closed connection's username from clientdetails.
	for(j=0;j<numofclients;j++)
       	  {				  
	    if(clientdetails[j].sd == i)					
	      {
 		   temp = j;	//Gets the position of username in the array of usernames
		   break;
	      }
	  }

	//Copy the username so that it can be notified to other users.
	strcpy(clientnames,clientdetails[temp].username);
	strcat(clientnames," has gone Offline.");

	//Remove the username from the array of usernames
 	for(j=temp;j<numofclients-1;j++)
	{
	  if(temp != numofclients-1)
	  {
	    strcpy(clientdetails[j].username,clientdetails[j+1].username);
	    clientdetails[j].sd = clientdetails[j+1].sd;
	  }	
	}				   
	//Decrement the number of active clients
	numofclients--;  

	//Construct the message from server side to notify the status of the disconnected client to other clients.
	servermessage.vrsn = 3;
    	servermessage.type = '6';//Offline message type
     	servermessage.length = 1;
        servermessage.attributes[0].type = 2;//username field
    	servermessage.attributes[0].payloadlength = strlen(clientnames);
     	strcpy(servermessage.attributes[0].payload,clientnames);
     	//printf("\nThe message is %s\n",clientnames);
 	for(k = 0; k <= fdmax; k++) {
             // send to everyone!
                   if (FD_ISSET(k, &master)) {
                   // except the listener and ourselves
                        if (k != sd && k != i && joinedornot[k] == 1) {
	 		  printf("\n Informing other clients of the user status\n");
                            if (write(k, (void *) &servermessage, sizeof(servermessage)) == -1) {
                                perror("send");
                            }
                        }
                    }
                 }

        bzero(clientnames,512); 
	 
}

void checkforvalidusername(struct clientattributes clientdetails[])
{
	
     for(j=0;j<numofclients;j++)
	{
	   //Check if the username already exists
	   if(strcmp(clientdetails[j].username,message.attributes[0].payload) == 0)
	    {	//Construct message to notify the client that the username already exists.
		strcpy(clientnames,"Username already exists. Please give another username.");
		validusername = 0;
		printf("\n Client Name Message is: %s",clientnames);
	     	servermessage.vrsn = 3;
	     	servermessage.type = '5';//NAK message type
	     	servermessage.length = 1;
                servermessage.attributes[0].type = 1;//Reason for failure field
	     	servermessage.attributes[0].payloadlength = strlen(clientnames);
	     	strcpy(servermessage.attributes[0].payload,clientnames);
	     	write(i, (void *) &servermessage, sizeof(servermessage));
		FD_CLR(i,&master);
                bzero(clientnames,512); 
		break;
	    }
	 }
}

void sendconnectedclients(struct clientattributes clientdetails[])
{
	joinedornot[i] = 1;//Update the joinedornot field to true				
        strcpy(clientdetails[numofclients].username,message.attributes[0].payload);
        clientdetails[numofclients].sd = i;
        numofclients++;
	
	//Construct the server message. Using sprintf as the int value needs to be added to the string buffer			
	//Client count field				    			 		
        sprintf(clientnames,"%d",numofclients-1);
       	strcat(clientnames," Users connected in the chat");
        			                                       				  
        //printf("\n Client Name Message is: %s",clientnames);
        servermessage.vrsn = 3;
        servermessage.type = '7';//ACK message type
        servermessage.length = 2;
        servermessage.attributes[0].type = 3;//username field
        servermessage.attributes[0].payloadlength = strlen(clientnames);
        strcpy(servermessage.attributes[0].payload,clientnames);
	bzero(clientnames,512);
	
	//Actual name of the active clients in the session
	strcpy(clientnames,"\n");
	for(j=0;j<numofclients-1;j++)
	{
	   strcat(clientnames,clientdetails[j].username);
	   strcat(clientnames,"\n");
	}	
	servermessage.attributes[1].type = 2;//Actual chat message field
        servermessage.attributes[1].payloadlength = strlen(clientnames);
        strcpy(servermessage.attributes[1].payload,clientnames);

        write(i, (void *) &servermessage, sizeof(servermessage));//Send to the newly connected client
        bzero(clientnames,512);  //Make it as 0 so that the same buffer can be reused again	

	//Construct server message to notify the other users that a new user has joind the chat.
	strcpy(clientnames,clientdetails[numofclients-1].username);
	strcat(clientnames," is Online.");	

	servermessage.vrsn = 3;
    	servermessage.type = '8';//Online message type
     	servermessage.length = 1;
        servermessage.attributes[0].type = 2;//username field
    	servermessage.attributes[0].payloadlength = strlen(clientnames);
     	strcpy(servermessage.attributes[0].payload,clientnames);
     //	printf("\nThe message is %s\n",clientnames);
 	for(k = 0; k <= fdmax; k++) {
             // send to everyone!
                   if (FD_ISSET(k, &master)) {
                   // except the listener and ourselves
                        if (k != sd && k != i && joinedornot[k] == 1) {
	 		  printf("\n Informing other clients of the user status\n");
                            if (write(k, (void *) &servermessage, sizeof(servermessage)) == -1) {//Broadcast to other clients
                                perror("send");
                            }
                        }
                    }
                 }

        bzero(clientnames,512); 
}
