
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

#define MYPORT 23557
#define LOCALHOST "127.0.0.1"
/* function to calcualate Propagation delay*/
double calculatePropagationDelay(double velocity,double length){
	//velocity in m/sec -> 10^7km/sec --> multiple by 10000 (10^7/10^3)
	velocity=velocity*10000; //(in km)
	return length/velocity;
}
/* function to convert dBm to watt*/
double convertowatt(double powerindbm){
	powerindbm=powerindbm/10;
	double powerinwatt = pow(10,powerindbm);
	return powerinwatt/1000;
}
/* function to calcualate SNR*/
double convertdBmtoRatio(double signal,double noise){
	//dbm to watt
	double signalwatt=convertowatt(signal);
	double noisewatt=convertowatt(noise);
	double snr=(signalwatt)/(noisewatt);
	return snr;
}
/*function to calculate channel capacity - transmission_rate*/
double calculateChannelCapacity(double bandwidth,double signal,double noise){
	double snr=convertdBmtoRatio(signal,noise);
	bandwidth=bandwidth*1000000;
	double logsnr=log10(1+snr)/log10(2);
	return bandwidth*logsnr;
}
int main(void)
{
	/*Socket creation, binding and recvfrom - adapted from Beej Socket Programming in C*/
	/*also references from https://www.cs.rutgers.edu/~pxk/rutgers/notes/sockets/*/
	struct sockaddr_in server_c_udp_addr;	/* server C address */
	struct sockaddr_in aws_udp_addr;	/* aws address */
	socklen_t addrlen = sizeof(aws_udp_addr);		/* length of addresses */
	int fd;				/* our socket */

	int numbytes;
	char s[INET6_ADDRSTRLEN];
	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}
	/* bind the socket to localhost and a specific port 23557*/
	memset((char *)&server_c_udp_addr, 0, sizeof(server_c_udp_addr));
	server_c_udp_addr.sin_family = AF_INET;
	server_c_udp_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	server_c_udp_addr.sin_port = htons(MYPORT);

	if (bind(fd, (struct sockaddr *)&server_c_udp_addr, sizeof(server_c_udp_addr)) < 0) {
		perror("bind failed");
		return 0;
	}
	printf("The Server C is up and running using UDP on port <%d>\n",MYPORT);
	/* now loop, receiving data and calculate the delays */
	while(1) {

		unsigned long long data[3];
	    if ((numbytes = recvfrom(fd, data, sizeof (data) , 0, (struct sockaddr *)&aws_udp_addr, &addrlen)) == -1) {
	        perror("recvfrom");
	        exit(1);
    	}
			printf("The Server C received link information of link <%d>, file size <%llu>, and signal power <%d>\n",(int)data[0],data[1],(int)data[2]);
    	// printf("Link id \"%d\"\n", (int)data[0]);
    	double bandwidth;
    	 if ((numbytes = recvfrom(fd, (char *)&bandwidth, sizeof (bandwidth) , 0, (struct sockaddr *)&aws_udp_addr, &addrlen)) == -1) {
	        perror("recvfrom");
	        exit(1);
    	}
    	// printf("bandwidth \"%fs\"\n", bandwidth);
    	double length;
    	if ((numbytes = recvfrom(fd, (char *)&length, sizeof (length) , 0, (struct sockaddr *)&aws_udp_addr, &addrlen)) == -1) {
	        perror("recvfrom");
	        exit(1);
    	}
    	// printf("length \"%f\"\n", length);
    	double velocity;
    	if ((numbytes = recvfrom(fd, (char *)&velocity, sizeof (velocity) , 0, (struct sockaddr *)&aws_udp_addr, &addrlen)) == -1) {
	        perror("recvfrom");
	        exit(1);
    	}
       // printf("velocity \"%f\"\n", velocity);
    	double noisepower;
    	if ((numbytes = recvfrom(fd, (char *)&noisepower, sizeof (noisepower) , 0, (struct sockaddr *)&aws_udp_addr, &addrlen)) == -1) {
	        perror("recvfrom");
	        exit(1);
    	}
	     // printf("Noise \"%f\"\n", noisepower);

		int signalpower=(int)data[2];
		double propagation_delay=calculatePropagationDelay(velocity,length);
		double transmission_rate=calculateChannelCapacity(bandwidth,signalpower,noisepower);
		double transmission_delay=data[1]/transmission_rate; //(file_size/data_rate)

		// printf("Caluclated transmission_delay as : %f\n",transmission_delay);
		// printf("Caluclated propagation_delay as : %f\n",propagation_delay);
		printf("The Server C finished the calculation for link <%d>\n",(int)data[0]);
		sendto(fd, &transmission_delay, sizeof(transmission_delay), 0, (struct sockaddr *)&aws_udp_addr, addrlen);
   		sendto(fd, &propagation_delay, sizeof(propagation_delay) , 0, (struct sockaddr *)&aws_udp_addr, addrlen);
		printf("The Server C finished sending the output to AWS\n");
	}
	close(fd);
}
