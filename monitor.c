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
#define SERVERPORT "26557"
#define BACKLOG 10
struct Delay{
    float t_trans;
    float t_prop;
    float end_to_end;
};

void *get_in_addr(struct sockaddr *sa)
{
return &(((struct sockaddr_in*)sa)->sin_addr);
}

int main(int argc, char *argv[])
{
  /*Socket creation, connect and recv- adapted from Beej Socket Programming in C*/
	int sockfd, numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(LOCALHOST, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
	if ((sockfd = socket(p->ai_family, p->ai_socktype,
		p->ai_protocol)) == -1) {
		perror("monitor: socket");
		continue;
		}
	if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		close(sockfd);
		perror("monitor: connect");
		continue;
	}
		break;
	}

	if (p == NULL) {
	fprintf(stderr, "monitor: failed to connect\n");
	return 2;
	}

	printf("The monitor is up and running\n");
	// printf("monitor: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure
	//run on loop - receive results from aws --exit when aws terminates
	char buffer[10];
	while(1){
		unsigned long long  data[3];
		int num=-1,n;
		printf("\n");
		if((n = send(sockfd, buffer, sizeof(buffer),0))==-1){
			close(sockfd);
			exit(0);
		}
		if((num=recv(sockfd, data, sizeof(data), 0))!=-1){
			printf("The monitor received input <%d>, file size <%llu>, and power <%d> from the AWS\n",(int)data[0],data[1],(int)data[2]);
		}
		if((n = send(sockfd, buffer, sizeof(buffer),0))==-1){
			close(sockfd);
			exit(0);
		}
		struct Delay result;
		if((num=recv(sockfd, &result, sizeof(result), 0))!=1){
			if(result.t_trans==-1){
				printf("Found no matches for link <%d>\n",(int)data[0]);
			}
			else{
		    printf("The result for the link <%d> :\n",(int)data[0]);
			printf("Tt: <%0.2f> ms\n",result.t_trans);
			printf("Tp : <%0.2f> ms\n",result.t_prop);
			printf("Delay : <%0.2f> ms\n",result.end_to_end);
			}
		}	
		printf("\n");
	}
	close(sockfd);
return 0;
}
