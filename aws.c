/*
** server.c -- a stream socket server demo
*/

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
#include <math.h>

#define LOCALHOST "127.0.0.1"
#define CLIENTTCP "25557" // the port users will be connecting to
#define MONITORTCP "26557"
#define MYUDP 24557
#define SERVERA 21557
#define SERVERB 22557
#define SERVERC 23557

#define BACKLOG 10     // how many pending connections queue will hold
//struct for link details
struct LinkDetail {
    int link;
    double bandwidth;
    double length;
    double velocity;
    double noisepower;
};
//struct for delay 
struct Delay{
    float t_trans;
    float t_prop;
    float end_to_end;
};

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//connect to backend servers A & B to get the link details
struct LinkDetail connectToBackendServer(char server, int linkid){

	int backendport;
	if(server=='A')
		backendport=SERVERA;
	else{
		backendport=SERVERB;
	}

    struct LinkDetail linkdetail={-1,-1,-1,-1,-1};
	
    struct sockaddr_in aws_udp_addr, backend_server;
    int fd, i, backserverlen=sizeof(backend_server);

    if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");

    memset((char *)&aws_udp_addr, 0, sizeof(aws_udp_addr));
    aws_udp_addr.sin_family = AF_INET;
    aws_udp_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    aws_udp_addr.sin_port = htons(MYUDP);

    if (bind(fd, (struct sockaddr *)&aws_udp_addr, sizeof(aws_udp_addr)) < 0) {
        perror("bind failed");
        return linkdetail;
    }       

    memset((char *) &backend_server, 0, sizeof(backend_server));
    backend_server.sin_family = AF_INET;
    backend_server.sin_port = htons(backendport);
    if (inet_aton(LOCALHOST, &backend_server.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    if (sendto(fd, &linkid, sizeof linkid, 0, (struct sockaddr *)&backend_server, backserverlen)==-1) {
            perror("sendto");
            exit(1);
        }
    printf("The AWS sent link ID=<%d> to Backend-Server %c using UDP over port <%d>\n",linkid,server,MYUDP);
    int m=0;
    if (recvfrom(fd, (char *)& m, sizeof m, 0 , (struct sockaddr *)&backend_server, &backserverlen)==-1) {
         perror("receiving detailss");
            exit(1);
    }
    char buff[5][100];
    if(m==1){
        recvfrom(fd, buff, sizeof(buff), 0 , (struct sockaddr *)&backend_server, &backserverlen);

        // printf("link id : '%s'\n",buff[0]);
        // printf("bandwidth : '%s'\n",buff[1]);
        // printf("length : '%s'\n",buff[2]);
        // printf("velocity : '%s'\n",buff[3]);
        // printf("noise power : '%s'\n",buff[4]);

        linkdetail.link=atoi(buff[0]);
        linkdetail.bandwidth=atof(buff[1]);
        linkdetail.length=atof(buff[2]);
        linkdetail.velocity=atof(buff[3]);
        linkdetail.noisepower=atof(buff[4]);
    }
    printf("The AWS received <%d> matches from Backend-Server <%c> using UDP over port <%d>\n",m,server,MYUDP);

    close(fd);
    return linkdetail;
}


//connect to compute server C and get the results
struct Delay sendInfoToComputerServer(struct LinkDetail detail,unsigned long long data[]){

    //printf("\nsending to server C\n");
    // printf("link id : '%d'\n",detail.link);
    // printf("bandwidth : '%d'\n",detail.bandwidth);
    // printf("length : '%f'\n",detail.velocity);
    // printf("velocity : '%f'\n",detail.length);
    // printf("noise power : '%f'\n",detail.noisepower);
    struct Delay delay;
    // printf("filesize : '%llu'\n",data[1]);
    // printf("signalpower : '%d'\n",(int)data[2]);
    /*also references from https://www.cs.rutgers.edu/~pxk/rutgers/notes/sockets/*/
    struct sockaddr_in aws_udp_addr, compute_server;
    int fd, i, backserverlen=sizeof(compute_server);

    if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");

    memset((char *)&aws_udp_addr, 0, sizeof(aws_udp_addr));
    aws_udp_addr.sin_family = AF_INET;
    aws_udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    aws_udp_addr.sin_port = htons(MYUDP);

    if (bind(fd, (struct sockaddr *)&aws_udp_addr, sizeof(aws_udp_addr)) < 0) {
        perror("bind failed");
        return delay;
    }       

    memset((char *) &compute_server, 0, sizeof(compute_server));
    compute_server.sin_family = AF_INET;
    compute_server.sin_port = htons(SERVERC);
    if (inet_aton(LOCALHOST, &compute_server.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    if (sendto(fd, data, 3*sizeof(unsigned long long), 0, (struct sockaddr *)&compute_server, backserverlen)==-1) {
            perror("sendto");
            exit(1);
        }
    if (sendto(fd, &detail.bandwidth, sizeof (detail.bandwidth), 0, (struct sockaddr *)&compute_server, backserverlen)==-1) {
            perror("sendto");
            exit(1);
        }
    if (sendto(fd, &detail.length, sizeof (detail.length), 0, (struct sockaddr *)&compute_server, backserverlen)==-1) {
            perror("sendto");
            exit(1);
        }
    if (sendto(fd, &detail.velocity, sizeof (detail.velocity), 0, (struct sockaddr *)&compute_server, backserverlen)==-1) {
            perror("sendto");
            exit(1);
        }
    if (sendto(fd, &detail.noisepower, sizeof (detail.velocity), 0, (struct sockaddr *)&compute_server, backserverlen)==-1) {
            perror("sendto");
            exit(1);
        }
    printf("The AWS sent link ID=<%d>, size=<%llu>, power=<%d> and link information to Backend-Server C using UDP over port <%d>\n",(int)data[0],data[1],(int)data[2],MYUDP);

    double t_trans;
    if (recvfrom(fd, & t_trans, sizeof(t_trans), 0 , (struct sockaddr *)&compute_server, &backserverlen)==-1) {
         perror("receiving detailss");
            exit(1);
    }
    t_trans*=1000;//to represent in ms
    float roundttrans=roundf(t_trans*100)/100;
    // printf("Transmission delay received as : %0.2f\n",roundttrans);
    double t_prop;
    if (recvfrom(fd, & t_prop, sizeof(t_trans), 0 , (struct sockaddr *)&compute_server, &backserverlen)==-1) {
         perror("receiving detailss");
            exit(1);
    }
    t_prop*=1000;//to represent in ms
    float roundtprop=roundf(t_prop*100)/100;
    // printf("Propagation delay received as : %0.2f\n",roundtprop);
    printf("The AWS received outputs from Backend-Server C using UDP over port <%d>\n",MYUDP);
    delay.t_trans=roundttrans;
    delay.t_prop=roundtprop;
    delay.end_to_end=roundttrans+roundtprop;
    return delay;
    close(fd);

}
/*bind and listen to TCP for client and monitor*/
  /*Socket creation, bind, listen adapted from Beej Socket Programming in C*/
int makeTCPconnection(char* port){
    int sockfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(LOCALHOST, port, &hints, &servinfo)) != 0) { //create a socket to listen to connections from client
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    return sockfd;

}

int main(void)
{
    socklen_t sin_size;
    struct sockaddr_storage client_addr; // connector's address information
    int new_client_fd,monitor_init_fd;
    struct sigaction sa;
    printf("The AWS is up and running\n");
    int clientfd = makeTCPconnection(CLIENTTCP);
    int monitorfd = makeTCPconnection(MONITORTCP);

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    monitor_init_fd = accept(monitorfd, (struct sockaddr *)&client_addr, &sin_size);
    if (monitor_init_fd == -1) {
        perror("accept");
    }
  /*Connection accpet, fork and close adapted from Beej Socket Programming in C*/
    while(1) {  // main accept() loop
        sin_size = sizeof client_addr;
        new_client_fd = accept(clientfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_client_fd == -1) {
            perror("accept");
            continue;
        }
        
//call 1. get details for link from db servers 2. get delay fom compute server C 3. send results to client and monitor
        if (!fork()) { // this is the child process
            close(clientfd); // child doesn't need the listener
            // close(monitorfd);
		    unsigned long long  data[3];
			recv(new_client_fd, data, sizeof(data), 0);
            printf("The AWS received link ID=<%d> , size=<%llu>, and power=<%d> from the client using TCP over port <%s>\n",(int)data[0],data[1],(int)data[2],CLIENTTCP);
			// printf("link id : '%llu'\n",data[0]);
			// printf("file size : '%llu'\n",data[1]);
			// printf("power : '%llu'\n",data[2]);
            send(monitor_init_fd, &data, sizeof(data), 0);
            printf("The AWS sent link ID=<%d> , size=<%llu>, and power=<%d> to the monitor using TCP over port <%s>\n",(int)data[0],data[1],(int)data[2],MONITORTCP);
			//open connection with backend server on UDP and send link id
            int found=0;
            struct LinkDetail detailA,detailB;
			detailA = connectToBackendServer('A',(int)data[0]);
    		detailB = connectToBackendServer('B',(int)data[0]);
            // printf("link id : '%d'\n",detail.link);
            // printf("bandwidth : '%f'\n",detail.bandwidth);
            // printf("length : '%f'\n",detail.velocity);
            // printf("velocity : '%f'\n",detail.length);
            // printf("noise power : '%f'\n",detail.noisepower);
            if(detailA.link!=-1){
                struct Delay e2edelay=sendInfoToComputerServer(detailA,data);
                send(new_client_fd, &e2edelay.end_to_end, sizeof(e2edelay.end_to_end), 0);
                printf("The AWS sent delay=<%f> to the client using TCP over port <%s>",e2edelay.end_to_end,CLIENTTCP);
                send(monitor_init_fd, &e2edelay, sizeof(e2edelay), 0);
                printf("The AWS sent detailed results to the monitor using TCP over port <%s>\n",MONITORTCP);
            }
            else if(detailB.link!=-1){
                struct Delay e2edelay=sendInfoToComputerServer(detailB,data);
                send(new_client_fd, &e2edelay.end_to_end, sizeof(e2edelay.end_to_end), 0);
                printf("The AWS sent delay=<%f> to the client using TCP over port <%s>\n",e2edelay.end_to_end,CLIENTTCP);
                send(monitor_init_fd, &e2edelay, sizeof(e2edelay), 0);
                printf("The AWS sent detailed results to the monitor using TCP over port <%s>\n",MONITORTCP);
            }
            else{
                struct Delay e2edelay={-1,-1,-1};
                send(new_client_fd, &e2edelay.end_to_end, sizeof(e2edelay.end_to_end), 0);
                send(monitor_init_fd, &e2edelay, sizeof(e2edelay), 0);
                printf("The AWS sent No Match to the monitor and client using TCP over ports <%s> and <%s> respectively\n",CLIENTTCP,MONITORTCP);
            }
            close(new_client_fd);
            // close(monitor_init_fd);
            exit(0);
        }
            close(new_client_fd);
            // close(monitor_init_fd);
    }

    return 0;
}

