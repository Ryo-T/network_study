#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "mpudp.h"


//#define PORT 8000
#define CPORT 8100
#define CADDRESS "127.0.0.1"

#define BUFFERSIZE 1024*1024
//#define MSGSIZE 11
#define KYE 200

#define TIMER 0//1秒
#define NANOTIMER 10//ナノ秒


void *memset(void *buf, int ch, size_t n);
int close(int fd);

/*
struct msglist{
	struct msglist *next;
	struct msglist *back;
	uint32_t id;
	uint16_t key;
	uint16_t length;
	char data[MSGSIZE+1];
};

struct socklist{
	int sock;
	struct sockaddr *addr;
	int addr_len;
};
*/

/*
struct connection_hdr{
	uint16_t key;
};
*/
struct socklist set_caddr(void){
	struct socklist sl;
	static struct sockaddr_in addr2;

	sl.sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sl.sock<0){
		printf("sl.sock err\n");
		return sl;
	}

	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(CPORT);
	addr2.sin_addr.s_addr = inet_addr(CADDRESS);

 	sl.addr = (struct sockaddr *)&addr2;
 	sl.addr_len = sizeof(addr2);

 	return sl;
}
/*
void free_msglist(struct msglist *ml){
	struct msglist *p = NULL;
	struct msglist *np = ml;

	do{
		p = np;
		np = np->next;

		printf("free [id=%d]\n",p->id);

		free(p);

	}while(np!=NULL);

	return;
}
*/
struct msglist *rebuild_msglist(char *buf){
	struct msglist *ml;
	char *p = buf;

	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error \n");
		return 0;
	}

	ml->next = NULL;
	ml->back = NULL;

	memcpy(&ml->id,p,sizeof(uint32_t));
	p = p + sizeof(uint32_t);
	memcpy(&ml->key,p,sizeof(uint16_t));
	p = p + sizeof(uint16_t);
	memcpy(&ml->length,p,sizeof(uint16_t));
	p = p + sizeof(uint16_t);
	memcpy(ml->data,p,ml->length);

	printf("id = %u\n",ml->id);
	printf("key = %u\n",ml->key);
	printf("length = %u\n",ml->length);
	printf("data = %s\n",ml->data);

	return ml;
}

struct msglist *accept_recvst(int fd,unsigned int flag){
	struct msglist *ml;
	int err = 0;
	char buf[BUFFERSIZE];

	err = recv(fd, buf, sizeof(buf), flag);
	if(err<0){
		printf("accept_recvst err\n");
		return NULL;
	}

	ml = rebuild_msglist(buf);
	if(!ml){
		printf("rebuild_msglist err\n");
		return NULL;
	}

	if(ml->id!=0){
		printf("id err\n");
		free_msglist(ml);
		return NULL;
	}

	printf("key = %u\n",(uint16_t)ml->key);

	return ml;
}

int do_recvst(int fd,struct msglist *front,unsigned int flag,uint8_t *ack_arr){
	struct msglist *ml;
	int err = 0;
	char buf[BUFFERSIZE];

	memset(buf,'\0',sizeof(buf));

	err = recv(fd, buf, sizeof(buf), flag);
	if(err<0){
		printf("accept_recvst err\n");
		return 0;
	}

	ml = rebuild_msglist(buf);
	if(!ml){
		printf("rebuild_msglist err\n");
		return 0;
	}

	if(ml->id==(uint32_t)~0){
		printf("close\n");
		return 0;
	}


// 再送チェックリスト
//----------------------------
	uint8_t *p = ack_arr;
	p = p + ml->id;
	*p = 1;
//	printf("*ack_arr + ml->id = %u\n",p);
//----------------------------
	front->next = ml;

	return 1;

}

void init_ack_arr(uint8_t *ack_arr,uint32_t ids){
	int i;
	uint8_t *p = ack_arr;

	// 配列0番目はコネクションパケット
	*p = 1;
	p++;

	for(i=1;i<=ids;i++){
		*p = 0;
		p++;
	}
	return;
}

// clientとかぶってる
void create_packet(char *buf,struct msglist *ml){
	char *bp = buf;

	memcpy(bp,&ml->id,sizeof(uint32_t));
	bp = bp + sizeof(uint32_t);
	memcpy(bp,&ml->key,sizeof(uint16_t));
	bp = bp + sizeof(uint16_t);
	memcpy(bp,&ml->length,sizeof(uint16_t));
	bp = bp + sizeof(uint16_t);
	memcpy(bp,ml->data,ml->length);

	return;
}

int rq_resendst(struct socklist *sl,uint8_t *ack_arr,uint32_t ids){
	int err = 0;
	uint8_t *p = ack_arr+sizeof(uint8_t);
	static struct msglist ml;
	char buf[13];// 32b + 16b + 16b + 32b
	int i;

	for(i=1;i<=ids;i++){
		printf("p = %u at %u\n",*p,p);
		if(*p == 1) {
			p = p + sizeof(uint8_t);
			continue;
		}

		memset(buf,'\0',sizeof(buf));

		ml.id = ~0-1;
		ml.length = sizeof(uint32_t);	
		ml.key = KYE;

		memcpy(ml.data,(char *)&i,sizeof(uint32_t));

		create_packet(buf,&ml);
		printf("id:%u,data:%s\n",i,ml.data);

		err = sendto(sl->sock,buf,sizeof(buf)-1,0,sl->addr,sl->addr_len);

		if(err<0){
			printf("Re sendto err\n");
//			return err;
		}

		p = p + sizeof(uint8_t);
	}



	return 1;
}

struct msghdr *get_resendst(int　fd,struct msglist *tail,uint8_t *ack_arr){
	struct msglist *mlp = tail;
	struct msglist *mlp_next;
	int err = 0;
	char buf[BUFFERSIZE];

// タイマーでちょっと待ち
//---------------------------------	
	struct timespec ts;		
	ts.tv_sec = TIMER;
	ts.tv_nsec = NANOTIMER;
	nanosleep(&ts, NULL);
//---------------------------------

	while(1){
		memset(buf,'\0',sizeof(buf));

		err = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
		if(err<0){
			printf("time out\n");
			break;
		}

		mlp_next = rebuild_msglist(buf);
		if(!mlp_next){
			printf("rebuild_msglist err\n");
			break;
		}

		if(mlp_next->id==(uint32_t)~0){
			printf("close\n");
			break;
		}


	// 再送チェックリスト
	//----------------------------
		uint8_t *p = ack_arr;
		p = p + mlp_next->id;

		printf("get packet id is [%u]\n", mlp_next->id);
		printf("get packet len is [%u]\n", mlp_next->length);
		printf("get packet key is [%u]\n", mlp_next->key);
		printf("get packet len is [%s]\n", mlp_next->data);

		// 既に取得しているパケットかどうかの確認
		if(*p==1){
			printf("alredy get [%u] packet\n",mlp_next->id);
			continue;
		}else{

			*p = 1;
		}
	//----------------------------

		// 再送されたパケットをリストへ追加
		mlp->next = mlp_next;
		mlp = mlp_next;

	}

	return mlp;

}

int check_ackarr(uint8_t *ack_arr,uint32_t ids){
	uint8_t *p = ack_arr;
	int counter = 0;
	int i;

	for (i = 1; i < ids; ++i){
		if(*p==0) counter++;

		p = p + sizeof(uint8_t);
	}

	return counter;
}

// クライアント側と同じ
int close_sendst(struct socklist *sl){
	int err = 0;
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;
	char buf[hsize];
	char *cp = buf;

	struct msglist ml;
	ml.id = ~0;
	ml.key = 0;
	ml.length = 0;

	memset(buf,'\0',sizeof(buf));
	create_packet(buf,&ml);

/*
	memcpy(cp,&ml.id,sizeof(uint16_t));
	cp = cp + sizeof(uint16_t);
	memcpy(cp,&ml->length,sizeof(uint16_t));
	cp = cp + sizeof(uint16_t);
	memcpy(cp,ml->data,connection_hdr_size);
*/
	err = sendto(sl[0].sock, buf, sizeof(buf), 0, sl[0].addr, sl[0].addr_len);

	if(!err){
		printf("close err \n");
		return 0;
	}

	return 1;
}

int collect_datas(void *ubuf,struct msglist *head,size_t bufsize){
	struct msglist *p = head;
	struct msglist *np = head->next;
//	char *b = ubuf;
	char *b;
	int size = 0;

	//memset(ubuf,'\0',sizeof(ubuf));
	memset(ubuf,'\0',bufsize);

	printf("buf size = %d\n",bufsize);

	do{
		p = np;
		np = np->next;

		size = size + (int)p->length;
		if(size >= bufsize){
			printf("over flow\n");
			return 0;
		}

		b = ubuf + MSGSIZE*(p->id-1);

		memcpy(b,p->data,p->length);
//		b = b + p->length;

		printf("[id=%u] [length=%u] data:%s\n",p->id,p->length,p->data);

	}while(np!=NULL);

	return size;

}

int recvst(int fd, void *ubuf, size_t size, unsigned int flag){
	int err = 0;
	int msgsize = 0;// all stream data size
	struct msglist *head;// first packet
	struct msglist *last;// first packet
	struct msglist *ml;

	// コネクション関数
	head = accept_recvst(fd,flag);// get connection packet
	if(!head)return -1;

	ml = head;

// 再送先定義
//--------------------------------------------
	
	struct socklist sl;
	sl = set_caddr();
	if(sl.sock<0) return -1;

//--------------------------------------------

// 再送チェック用配列の定義(仮)
//---------------------
	uint32_t ids = 0;// how many packets will be come
	memcpy(&ids,head->data,sizeof(uint32_t));
	if(ids<1) return -1;
	printf("ids = %d\n",ids);

	uint8_t *ack_arr;
	// uint32サイズの配列無いからメモリで配列っぽいものを作成
	if ((ack_arr = (uint8_t *) malloc(sizeof(uint8_t)*ids)) == NULL) {
		printf("malloc error \n");
		return -1;
	}

	init_ack_arr(ack_arr,ids);
//	printf("*ack_arr = %u\n\n",ack_arr );

//	printf("plist = %u\n",(uint8_t)*(ack_arr));

//---------------------


	// データ部受信
	while(1){

		err = do_recvst(fd,ml,flag,ack_arr);
		if(!err) break;

		ml = ml->next;

		printf("id=======%u\n",ml->id);

	}

	// 受信データがなければ終了
	if(!head->next) return -1;

//再送処理
//----------------------------------------------------

	while(1){

		err = rq_resendst(&sl,ack_arr,ids);

		if(err<0){
			printf("rq_resendst = err\n");
			break;
		}

//		ml = get_resendst(fd,ml,ack_arr);
		ml = get_resendst(fd,ml,ack_arr);
		err = check_ackarr(ack_arr,ids);

		printf("check counter = %d\n", err);

		if(err==0){
			printf("get all packet = %d\n",err);
			break;
		}
		
	}

	// 終了
	err = close_sendst(&sl);
	printf("close_sendst = %d\n",err );


//----------------------------------------------------


	printf("head->id:%u\n",head->next->id);

	//再送用ソケットの開放
	close(sl.sock);

	// リストのデータのをまとめる
	msgsize = collect_datas(ubuf,head,size);

	// リストのメモリ解放
	free_msglist(head);

	// チェック配列のメモリの開放
	free(ack_arr);

	return msgsize; 
}

/*
int main(){
	int sock;
	struct sockaddr_in addr;
	char buf[BUFFERSIZE];
	int err = 0;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(!sock){
		printf("sock err\n");
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	err = bind(sock, (struct sockaddr *)&addr,sizeof(addr));

	if(err<0){
		printf("bind err\n");
		return 0;
	}



	//while(1){

		memset(buf, '\0', sizeof(buf));

		err = recvst(sock,buf,sizeof(buf),0);

//		err = recv(sock, buf, sizeof(buf), 0);

		if(err<0){
			printf("recv err\n");
		}

	printf("<<stream data>>\n%s\n", buf);
	printf("data len = %d\n",err);

	//}


	close(sock);

	return 0;
}

*/