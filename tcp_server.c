#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#define PORT 8000
#define BUFFERSIZE 1024*1024


int main(){
	int sock0;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	char data[BUFFERSIZE];
	char buf[BUFFERSIZE];
	char *p;
	int len;
	int sock;
	int err = 0;
	int size = 0;

	struct timeval s,e;

	/* ソケットの作成 */
	sock0 = socket(AF_INET, SOCK_STREAM, 0);
	if(sock0 < 0){
		printf("sock err\n");
		return 0;
	}

	/* ソケットの設定 */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	err = bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	if(err < 0){
		printf("bind err\n");
		return 0;
	}


	while(1){

	/* TCPクライアントからの接続要求を待てる状態にする */
	err = listen(sock0, 5);
	if(err<0){
		printf("listen err\n");
		return 0;
	}

	/* TCPクライアントからの接続要求を受け付ける */
	len = sizeof(client);
	sock = accept(sock0, (struct sockaddr *)&client, &len);
	if(sock < 0){
		printf("accept err\n");
		return 0;
	}

	gettimeofday(&s,NULL);

//	printf("accept ok\n");
/*
	// 5文字送信
	err = write(sock, "HELLO", 5);
	if(err < 0){
		printf("write err\n");
		return 0;
	}
*/
	// サーバからデータを受信 
	memset(buf, 0, sizeof(buf));
	p = buf;
	size = 0;

	while(1){

		memset(data,'\0',sizeof(buf));

		err = read(sock, data, sizeof(buf));
		if(err < 1){
			break;
		}

		printf("data = %d\n",err);
		memcpy(p,data,err);

		p = p + err;
		size = size + err;
	}


//	write(sock0,"END",3);
	gettimeofday(&e,NULL);

//	printf("%s\n", buf);

	printf("size:%d\n",size);

//	gettimeofday(&e,NULL);
	printf("time = %f\n",(e.tv_sec - s.tv_sec)+(e.tv_usec - s.tv_usec)*1.0E-6);


	/* TCPセッションの終了 */
	close(sock);

	}

	/* listen するsocketの終了 */
	close(sock0);


	return 1;
}
