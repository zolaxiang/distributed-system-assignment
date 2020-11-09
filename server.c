#include <errno.h>       /* obligatory includes */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

void process(char *a)
{
	int len=strlen(a);
	int flag=1, i;
	for(i=0; i< len; i++)
	{
		if(a[i]==9 || a[i]==32)
			flag =1;
		else
		{
			if(flag)
			{
				a[i]=toupper(a[i]);
				flag=0;
			}
			else
				a[i]=tolower(a[i]);
		}
	}
}
int main()
{
	fd_set master; 
	fd_set read_fds; //file descriptor sets.
	int fd_max;
	int listener, new_fd;

	int i;
	char buf[10000];

	struct sockaddr_storage their_addr;
	socklen_t addr_size;   //these two are for accepted sockets

	struct addrinfo hints, *res;

	// first, load up address structs with getaddrinfo():
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	getaddrinfo(NULL, "0", &hints, &res);
	
	//create socket and bind
	listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	bind(listener, res->ai_addr, res->ai_addrlen);

	//print
	FILE *fp;
	char name[100];
	fp = popen("hostname -f", "r");
	fgets(name, 99, fp);
	printf("SERVER_ADDRESS %s", name);

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	getsockname(listener, (struct sockaddr *)&sin, &len);
	printf("SERVER_PORT %d\n", ntohs(sin.sin_port));
		
	//
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(listener, &master);
	fd_max = listener;
	//
	listen(listener, 10);
//	printf("listener:%d\n", listener);
	while(1)
	{
		read_fds = master;
		select(fd_max+1, &read_fds, NULL, NULL, NULL);

		for(i=0; i<=fd_max; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{
				 if (i == listener) 
				 {
					addr_size = sizeof their_addr;
					new_fd = accept(listener, (struct sockaddr *)&their_addr, &addr_size);
					FD_SET(new_fd, &master);
					if(new_fd> fd_max)
						fd_max=new_fd;
			//		printf("new_fd:%d\n", new_fd);
				 }
				 else
				 {
					char bytes[5];
					if(recv(i, bytes, 4, 0) ==0)
					{	
						close(i);
						FD_CLR(i, &master);
			//			printf("closed\n");
					}
					else
					{
						int len= bytes[0]+ bytes[1]*256 + bytes[2]* 65536 + bytes[3]*16777216;
			//			printf("len: %d\n", len);
						int received=0;
						while (received < len)
						{
							int r = recv(i, &buf[received], len-received, 0);
							received += r;
						}
						printf("%s", buf);
						process(buf);
						send(i, bytes, 4, 0);
						send(i, buf, len, 0);
					}
				 }
			}
		}
	}
}