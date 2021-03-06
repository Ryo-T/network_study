#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//#define BUFFERSIZE 32
#define PORT 8000
#define ADDRESS "192.168.211.150"


// get file data size
long get_file_size(const char *file[]){
	fpos_t fsize;
	long sz = 0;

	FILE *fp = fopen(file,"rb");

	fseek(fp,0,SEEK_END);
//	fgetpos(fp,&fsize); 
	sz = ftell(fp);

	printf("FILESIZE = %lo\n",sz);

	fclose(fp);

	return sz;
}


int main(){
	struct sockaddr_in server;
	int sock;
//	char buf[BUFFERSIZE];
	int n;
	int err = 0;
	FILE *fp;
	char *file = "./sample2.txt";
	long flen = 0;
	char buf2[16];

// get file data into the buffer
//--------------------------------------
	fp = fopen(file,"r");
	if(!fp){
		printf("file open err \n");
		return 0;
	}

	// get file size
	flen = get_file_size(file);
	printf("%s=%d\n",file,flen);

	// crate buffer
	char buf[flen+1];

	// file data into buffer
	fread(buf,flen,1,fp);

//---------------------------------------

	/* ソケットの作成 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		printf("sock err\n");
		return 0;
	}

	/* 接続先指定用構造体の準備 */
	server.sin_family = AF_INET; 
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr(ADDRESS);

//	setsockopt(sock, SOL_SOCKET,SO_MAX_MSG_SIZE, "11", 2);

	/* サーバに接続 */
	err = connect(sock, (struct sockaddr *)&server, sizeof(server));
	if(err < 0){
		printf("connect err\n");
		return 0;
	}

//	printf("connection ok\n");

/*
	// サーバからデータを受信 
	memset(buf, 0, sizeof(buf));
	err = read(sock, buf, sizeof(buf));
	if(err < 0){
		printf("read err\n");
		return 0;
	}
*/

	err = write(sock, buf, flen);
	if(err < 0){
		printf("write err\n");
		return 0;
	}


	printf("err:%d\n",err);

	/* socketの終了 */
	close(sock);

	fclose(fp);

	return 1;
}
