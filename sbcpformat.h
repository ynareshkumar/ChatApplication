#include<stdio.h>

//SBCP Attribute fields
struct sbcpattributes{
  int type :16;//Type can be 1-Reason,2-username,3-client count,4-Actual message
  int payloadlength :16;//Length of payload[512]
  char payload[512];//Actual message to be sent
};

//SBCP Message format
struct sbcpmessage{
  int vrsn :9;//Protocol version
  char type :7;//Message type can be 2-JOIN,3-FWD,4-SEND
  int length :16;//Number of payloads
  struct sbcpattributes attributes[4];//Array of attribute fields
};

//Client attributes
struct clientattributes{
  char username[16];//Username/Nick name of the client
  int sd;//Socket id for that client
};
