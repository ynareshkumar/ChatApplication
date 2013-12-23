Overview:		
----------

Simple Broadcast Chat Protocol(SBCP) is a protocol that allows multiple clients to connect to a server and chat with each other.

System Requirements:
---------------------
The application should be executed in the Linux Environment. Gcc should be installed in the machine for compiling the .c files.


Features of the Chat Application :
----------------------------------

1. A single server can handle connections from multiple clients. 
2. The maximum number of clients that the server can handle is given as a command line argument while starting the server.
3. The client can chat with the other clients connected in the current chat session. The message will be reeceived by the server and broadcasted to the other clients.
4. Each client is identified by a unique username and no 2 clients can have the same username.
5. When a new client successfully joins the chat session, the client will be informed the number of clients apart from it who has already joined the chat session. It will also be displayed the usernames of each other client in the chat session.
6. When a new user joins or a existing user quits, the other clients are informed of the status of that user.(i.e)Online or Offline.


Connection refusal scenarios:
------------------------------

1. When a new client tries to join a chat session and already the maximum number of clients is reached, the client will be asked to try later.
2. When a client tries to join a chat session with a username which is already present in the list of active clients, connection will be refused.
3. When server is disconnected, the clients will be informed of the same and connection will be closed.

Client side Commands:
----------------------

1. To join a chat session, type JOIN and press Enter when in prompt for the first time.
2. To exit from the chat, press Esc key and Enter or press Ctrl+C.

How to run the Application:
----------------------------

1. Copy newserver.c,client.c,sbcpformat.h and makefile to a folder.
2. Go to that folder in terminal and type make.
3. Run the server by executing ./newserver <Server-IP> <Server-Port> <Maximum-number-of-clients>
   Eg: ./newserver 127.0.0.1 5000 5
4. Open another terminal and run the client by executing ./client <username> <Server-IP> <Server-Port>
   Eg: ./client naresh 127.0.0.1 5000
NOTE: The username cannot contain spaces and cannot exceed 16 characters.
5. Repeat step 4 to execute multiple clients.
6. After the client is executed, the terminal will prompt for input. Type JOIN and press Enter to join the group chat session.
7. Now the client can send and receive messages to/from other clients.


NOTE: After typing the command make in terminal,sometime you may find messages like:nothing to be done or 'all'. If such problem happens, you can type "touch client.c" and "touch newserver.c",then re-type make command.
