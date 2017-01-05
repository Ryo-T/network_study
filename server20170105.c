#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void *memset(void *buf, int ch, size_t n);
int close(int fd);

int main(){
	int sock;
	struct sockaddr_in addr;
	char buf[2048];
	int err = 0;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(!sock){
		printf("sock err\n");
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = INADDR_ANY;

	err = bind(sock, (struct sockaddr *)&addr,sizeof(addr));

	if(err<0){
		printf("bind err\n");
		return 0;
	}



	while(1){

		memset(buf, '\0', sizeof(buf));

		err = recvst(sock,buf,sizeof(buf));

//		err = recv(sock, buf, sizeof(buf), 0);

		if(err<0){
			printf("recv err\n");
		}

	printf("<<stream data>>\n%s\n", buf);
	printf("data len = %d\n",err);

	}


	close(sock);

	return 0;
}