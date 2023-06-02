#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

//socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//display error msg
void error_msg(char*msg);
//setup SOCKET
int setup_socket(int IP_ver);
//setup server address
void setup_address(char*SERVER_IP, int SERVER_PORT, struct sockaddr_in6*storeAddr);
//get scope id from link local address

int getLink_local_addrSCOPEID(char *LINK_LOCAL_ADDR);
//interact with client
void interact_with_client(int serverSock);

int main(int argc, char *argv[]){
 	//arguemtns
	char SERVER_IP[50]={0};
	int SERVER_PORT=0;

	//socket - server
	int serverSock=0;
	struct sockaddr_in6 serverAddr;

    	//socket - client
    	int clientSock=0;
	//process arguments
	if(argc!=3){
		error_msg("[Usage] TCP_server SERVER_IP SERVER PORT (SERVER_IP6 : IP%IF_NAME)" );
	}
	strncpy(SERVER_IP, argv[1], 50-1);
	//check interface name
	if(strchr(SERVER_IP,'%' )==NULL) {
		error_msg("[ERROR] SERVER_IP6 should be IP: IF_NAME");
	}
	SERVER_PORT=atoi(argv[2]);

	//setup server socket

	serverSock = setup_socket(6);

	memset(&serverAddr,0, sizeof(serverAddr) );
	setup_address(SERVER_IP, SERVER_PORT, &serverAddr);

	if(bind(serverSock,(struct sockaddr*)&serverAddr, sizeof (serverAddr))<0){
		error_msg("[ERROR] Failed to bind.");
	}
	listen(serverSock, 2);
	printf("[INFO] server setup %s[%d]\n", SERVER_IP, SERVER_PORT);

	//interact with client
	interact_with_client(serverSock);

	//close socket
	close(serverSock);

	return 0;
}

//error message
void error_msg(char*msg){
	fprintf(stderr,"%s %s\n",msg,strerror(errno));
	exit(1);
}

//setup socket
int setup_socket(int IP_ver){
	int sockFd;
	int sockopt=1;

	sockFd = socket(IP_ver==4 ? PF_INET : PF_INET6, SOCK_STREAM, 0);
	setsockopt(sockFd, SOL_SOCKET,SO_REUSEADDR,&sockopt, sizeof(sockopt));
	return sockFd;
}
//setup server address (only for IPv6)
void setup_address(char*SERVER_IP,int SERVER_PORT,struct sockaddr_in6*storeAddr) {
	char SERVER_IP_CHR[50]={0};

	strncpy(SERVER_IP_CHR,SERVER_IP, (size_t)(strchr(SERVER_IP,'%' )-SERVER_IP));

	storeAddr->sin6_family = AF_INET6;
	inet_pton(AF_INET6,SERVER_IP_CHR,&storeAddr->sin6_addr);
	storeAddr->sin6_port = htons(SERVER_PORT);
	storeAddr->sin6_scope_id=getLink_local_addrSCOPEID(SERVER_IP);

	return;
}

//get scope id from link Ss
int getLink_local_addrSCOPEID(char *LINK_LOCAL_ADDR){
	int scope_ID=-1;

	//getaddrinfo
	struct addrinfo hints, *info;

	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_NUMERICHOST;
	//int getaddrinfo(const char *node, const C
	if(getaddrinfo(LINK_LOCAL_ADDR, NULL, &hints,&info)==0){
		struct sockaddr_in6 * sin6_info = (struct sockaddr_in6*)info->ai_addr;
		scope_ID = sin6_info->sin6_scope_id;
		freeaddrinfo(info);

	}

	return scope_ID;
}

void interact_with_client(int serverSock){
		//socket - client

	int clientSock=0;
	struct sockaddr_in6 clientAddr;

	int clientAddrLength = sizeof(clientAddr);
	char clientAddr_str[50]={0};

	//message
	char msg[BUFSIZ]={0};

	while(1){
		//wait for client
		clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientAddrLength);
		inet_ntop(AF_INET6, &clientAddr.sin6_addr, clientAddr_str, sizeof(clientAddr_str));
		printf("[INFO] Connection from %s[%d]\n", clientAddr_str, ntohs(clientAddr.sin6_port));

		while(1){
			//receive message from server
			memset(msg,0, sizeof(msg));
			if(recv(clientSock,&msg,sizeof(msg),0)<=0){
				printf("[ INFO] Client disconnected. \n" );
				break;
			}
			printf(" [CLIENT] %s\n", msg);

			strcat(msg, "[server readed]");
			//send message back to client
			send(clientSock,msg,strlen(msg), 0);
		}
	}
	return;
}
