#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define LOCALHOST "127.0.0.1"
#define SERVERPORT "25557"

void *get_in_addr(struct sockaddr *sa)
{
return &(((struct sockaddr_in*)sa)->sin_addr);
}

int main(int argc, char *argv[])
{
	unsigned long long data[3];
	unsigned long long example;
  int i;
    for (i = 1; i < argc; i++) {
        sscanf(argv[i], "%llu", &example);
        data[i-1]=example;
    }
    // printf("link id : %llu \n",data[0]);
    // printf("file size : %llu \n",data[1]);
    // printf("signal power : %llu \n",data[2]);
	int sockfd, numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	/*Socket creation, connect and recv- adapted from Beej Socket Programming in C*/
	if ((rv = getaddrinfo(LOCALHOST, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
	if ((sockfd = socket(p->ai_family, p->ai_socktype,
		p->ai_protocol)) == -1) {
		perror("client: socket");
		continue;
		}
	if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		close(sockfd);
		perror("client: connect");
		continue;
	}
		break;
	}

	if (p == NULL) {
	fprintf(stderr, "client: failed to connect\n");
	return 2;
	}
	printf("The client is up and running\n");
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	// printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure

	//send link id to aws server
	send(sockfd, data, sizeof(data), 0);
	printf("The client sent link ID=<%d> , size=<%llu>, and power=<%d> to AWS\n",(int)data[0],data[1],(int)data[2]);
	float delay = 0;

	//receive delay from AWS
	recv(sockfd, &delay, sizeof delay, 0);
	if(delay==-1){
		printf("Found no matches for link <%d>\n",(int)data[0]);
	}
	else{
	printf("The delay for link <%d> is <%0.2f>ms\n", (int)data[0],delay);
	}

	close(sockfd);
return 0;
}
