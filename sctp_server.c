#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>


#define PORT 8000
#define BUFFERSIZE 1024*1024


int main(){
	int sock0;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	char data[BUFFERSIZE];
	char buf[BUFFERSIZE];
	char *p = buf;
	int len;
	int sock;
	int err = 0;
	int size = 0;

	/* ソケットの作成 */
	sock0 = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
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


	struct sctp_initmsg sctp_init;
	memset(&sctp_init,0,sizeof(sctp_init));
	sctp_init.sinit_num_ostreams = 10;
	sctp_init.sinit_max_instreams = 10;
	sctp_init.sinit_max_attempts = 0;
	sctp_init.sinit_max_init_timeo = 0;
	setsockopt(sock0,IPPROTO_SCTP,SCTP_INITMSG,&sctp_init,sizeof(sctp_init));



	/* TCPクライアントからの接続要求を待てる状態にする */
	err = listen(sock0, 5);
	if(err<0){
		printf("listen err\n");
		return 0;
	}

	/* TCPクライアントからの接続要求を受け付ける */
	len = sizeof(client);
//	sock = accept(sock0, (struct sockaddr *)&client, &len);
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


	int flags;
	struct sctp_sndrcvinfo sndrcvinfo;

	while(1){

		memset(data,'\0',sizeof(buf));

//		err = read(sock, data, sizeof(buf));
		err = sctp_recvmsg(sock,data,sizeof(data),
				(struct sockaddr *)NULL,0,&sndrcvinfo,&flags);
		if(err < 1){
			break;
		}

		printf("err = %d\n",err);
		memcpy(p,data,err);

		p = p + err;
		size = size + err;
	}

	printf("%s\n", buf);

	printf("size:%d\n",size);

	/* TCPセッションの終了 */
	close(sock);

	/* listen するsocketの終了 */
	close(sock0);

	return 1;
}
