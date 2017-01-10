#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8000
#define BUFFERSIZE 1024*1024


int main(){
	int sock0;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	char buf[BUFFERSIZE];
	int len;
	int sock;
	int err = 0;

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
	err = read(sock, buf, sizeof(buf));
	if(err < 0){
		printf("read err\n");
		return 0;
	}

	printf("%s\n", buf);

	/* TCPセッションの終了 */
	close(sock);

	/* listen するsocketの終了 */
	close(sock0);

	return 1;
}
