
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MYPORT 21557
#define LOCALHOST "127.0.0.1"

int main(void)
{
	struct sockaddr_in server_a_udp_addr;	/* server A address */
	struct sockaddr_in aws_udp_addr;	/* aws address */
	socklen_t addrlen = sizeof(aws_udp_addr);		/* length of addresses */
	int fd;				/* our socket */

	int numbytes;
	char s[INET6_ADDRSTRLEN];
	/*Socket creation, binding and recvfrom - adapted from Beej Socket Programming in C*/
	/*also references from https://www.cs.rutgers.edu/~pxk/rutgers/notes/sockets/*/
	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}
	/* bind the socket to localhost and a specific port 21557*/
	memset((char *)&server_a_udp_addr, 0, sizeof(server_a_udp_addr));
	server_a_udp_addr.sin_family = AF_INET;
	server_a_udp_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	server_a_udp_addr.sin_port = htons(MYPORT);

	if (bind(fd, (struct sockaddr *)&server_a_udp_addr, sizeof(server_a_udp_addr)) < 0) {
		perror("bind failed");
		return 0;
	}
	printf("The Server A is up and running using UDP on port <%d>\n",MYPORT);
	/* now loop, receiving data and printing what we received */
	while(1) {
		//received the link id
		int linkid;
	    if ((numbytes = recvfrom(fd, (char *)&linkid, sizeof linkid , 0, (struct sockaddr *)&aws_udp_addr, &addrlen)) == -1) {
	        perror("recvfrom");
	        exit(1);
    	}
		//looking up the link id in the file and returning the link details if found
		printf("The Server A received input <%d>\n",linkid);
  		FILE* stream = fopen("database_a.csv", "r");

	   char line[1024];
	   char data[5][100];
	    int found=0;
	    while (fgets(line, 1024, stream))
	    {
	    char* tok;
	    tok = strtok(line, ",");
	    if(atoi(tok)==linkid){
	    	// printf("\nBackend server A found the link..");
	    	found=1;
	    	int i=0;
	    	for (;tok && *tok;tok = strtok(NULL, ",\n"))
		    {
	    		strcpy(data[i++],tok);
		    	// printf("\nlink detail %d is %s",i,tok);
		    }
		    break;
	    }
		}
	    int m=1;
	    if(found==1){
	    	printf("The Server A has found < %d > matches\n",m);
				//sending details of that link to the aws server
	    	sendto(fd, &m, sizeof m, 0, (struct sockaddr *)&aws_udp_addr, addrlen);
	    	sendto(fd, data, sizeof data , 0, (struct sockaddr *)&aws_udp_addr, addrlen);
	    	printf("The Server A finished sending the output to AWS\n");
	    }
	    else{
				m=0;
	    	printf("The Server A has found < %d > matches",m);
	    	sendto(fd, &m, sizeof m, 0, (struct sockaddr *)&aws_udp_addr, addrlen);
	    }
	    printf("\n");
	}
	close(fd);
}
