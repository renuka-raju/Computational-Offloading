all:
		gcc -o serverAoutput serverA.c
		gcc -o serverBoutput serverB.c
		gcc -o serverCoutput serverC.c -lm
		gcc -o awsoutput aws.c -lm
		gcc -o monitoroutput monitor.c
		gcc -o client client.c

serverA:
		./serverAoutput
serverB:
		./serverBoutput
serverC:
		./serverCoutput
aws:
		./awsoutput
monitor:
		./monitoroutput