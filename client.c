#include <errno.h>      
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define capacity 50
#define max_length 10000

char *a[capacity];
int head=0, tail=0, num=0, end_program=0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *input()
{
	int len;
    while(1)
    {
		char temp[max_length];
        if(fgets(temp, max_length, stdin)==NULL)
		{
			end_program=1; //EOF
			continue;
		}
		len=strlen(temp);
		a[tail] = (char *) malloc(len+1);
		strcpy(a[tail], temp);

        tail=(tail+1)%capacity;
        pthread_mutex_lock (&mtx);
        num++;
        pthread_mutex_unlock (&mtx);
    }
}

int main()
{
	struct addrinfo hints, *res, *p;
	int sockfd;

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;

	char *addr;
	char *port;
	addr = getenv ("SERVER_ADDRESS");
	port = getenv ("SERVER_PORT");
	getaddrinfo(addr, port, &hints, &res);

	// make a socket:

	for(p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1)
            continue;
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }
	
	//
	pthread_t thread;
    pthread_create(&thread, NULL, input, NULL);
	//
	char buf[max_length];
	while(1){		
		if(end_program && num==0) //EOF
			exit(0);
		if(num>0)
        {
				int i, len=strlen(a[head])+1;
				char bytes[4];
				for(i=0;i<4;i++)
					bytes[i]= (len >> (8*i)) & 0xff;

				send(sockfd, bytes, 4, 0);
                send(sockfd, a[head], len, 0);

				head=(head+1)%capacity;
				pthread_mutex_lock (&mtx);
                num--;
                pthread_mutex_unlock (&mtx);

				recv(sockfd, bytes, 4, 0);
				int received=0;
				while (received < len)
				{
					int r = recv(sockfd, &buf[received], len-received, 0);
					received += r;
				}
				printf("Server: %s", buf);
                sleep(2);
        }
	}
}