#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

// 受信ポート
#define PORT 8000
#define PORT2 8001
// 再送パケット送信ポート
#define CPORT 8100
#define CPORT2 8101
// 再送パケット送信アドレス
#define CADDRESS "192.168.211.134"
#define CADDRESS2 "192.168.211.135"

#define BUFFERSIZE 1024*1024*2
#define MSGSIZE 9000
#define KYE 200

// インターフェース関連
#define MAXIF 10 // default 10
#define HOW_MANY_IF 2
//#define IF_LIST "lo0","lo0"
#define IF_LIST "ens38","ens33"
//#define WORDCOUNT 3,3
#define WORDCOUNT 5,5

// TIMEOUT関連
#define TIMEOUT 1000
#define STIMER 0//1秒
#define NANOTIMER 10//ナノ秒

#define __RE__TIMEOUT 1000
#define __RE__STIMER 0//1秒
#define __RE__NANOTIMER 10//ナノ秒

void *memset(void *buf, int ch, size_t n);
int close(int fd);

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

// 再送先のアドレス設定
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

// 受信サブソケットの生成
struct socklist set_saddr(int port,char *dev,int wcount){
	int err = 0;
	struct socklist sl;
	static struct sockaddr_in addr2;

	sl.sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sl.sock<0){
		printf("sl.sock err\n");
		return sl;
	}

	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(port);
	addr2.sin_addr.s_addr = INADDR_ANY;

 	sl.addr = (struct sockaddr *)&addr2;
 	sl.addr_len = sizeof(addr2);

 	err = bind(sl.sock, (struct sockaddr *)&addr2,sizeof(addr2));

	if(err<0){
		printf("bind err\n");
		return sl;
	}

	setsockopt(sl.sock, SOL_SOCKET, SO_BINDTODEVICE, dev, wcount);

 	return sl;
}

// msglistの開放
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

// 受信パケットからmsglistを再構成
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

	//printf("rebuild:id = %u\n",ml->id);
	//printf("rebuild:key = %u\n",ml->key);
	//printf("rebuild:length = %u\n",ml->length);
	//printf("rebuild:data = %s\n",ml->data);

	return ml;
}

// コネクションパケットの受信
struct msglist *accept_recvst(int fd,unsigned int flag){
	struct msglist *ml;
	int err = 0;
	char buf[BUFFERSIZE];

	err = recv(fd, buf, sizeof(buf), flag);
	if(err<0){
		printf("accept_recvst err\n");
		return NULL;
	}

	// msglist再構成
	ml = rebuild_msglist(buf);
	if(!ml){
		printf("rebuild_msglist err\n");
		return NULL;
	}

	// idが0でなければ終了
	if(ml->id!=0){
		printf("id err\n");
		free_msglist(ml);
		return NULL;
	}

	printf("key = %u\n",(uint16_t)ml->key);

	return ml;
}

// データ受信
int do_recvst(int sock,struct msglist *front,unsigned int flag,uint8_t *ack_arr){
	struct msglist *ml;
	int err = 0;
	char buf[BUFFERSIZE];

	memset(buf,'\0',sizeof(buf));

//	err = recv(fd, buf, sizeof(buf), flag);
	err = recv(sock, buf, sizeof(buf), MSG_DONTWAIT);
	//err = recvfrom(recvsl->sock, recvbuf, sizeof(recvbuf), MSG_DONTWAIT,recvsl->addr, &recvsl->addr_len);
	if(err<0){
		printf("recvst err\n");
		return 2;
	}

	// msglist再構成
	ml = rebuild_msglist(buf);
	if(!ml){
		printf("rebuild_msglist err\n");
		return 0;
	}

	// idが0の反転ビットならば終了
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

//if関連
//---------------------------------------
//	if(HOW_MANY_IF!=0){
//		if_num = (if_num+1)%HOW_MANY_IF;
//	}
//	printf("IF = %d\n",if_num);
//---------------------------------------


	front->next = ml;

	return 1;

}

// 受信パケットリストの初期化
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

// msglistからパケットの生成(clientとかぶってる)
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

// 再送要求
int rq_resendst(struct socklist *sl,uint8_t *ack_arr,uint32_t ids){
	int err = 0;
	uint8_t *p = ack_arr+sizeof(uint8_t);
	static struct msglist ml;
	char buf[13];// 32b + 16b + 16b + 32b
	uint32_t i;
	void *ip;

	// 0はコネクションパケットidなので飛ばす
	for(i=1;i<=ids;i++){
		//printf("p = %u at %u\n",*p,p);
		if(*p == 1) {
			p = p + sizeof(uint8_t);
			continue;
		}

		memset(buf,'\0',sizeof(buf));
		memset(ml.data,'\0',sizeof(ml.data));

		// 再装用idを指定
		ml.id = ~0-1;
		ml.length = sizeof(uint32_t);	
		ml.key = KYE;

		memcpy(ml.data,&i,sizeof(uint32_t));

		create_packet(buf,&ml);
//		printf("[request]id:%u,data:%u\n",i,(uint32_t)ml.data);

		// 再送要求
		err = sendto(sl->sock,buf,sizeof(buf)-1,0,sl->addr,sl->addr_len);

		if(err<0){
			printf("Re sendto err\n");
			//return err;
		}

		p = p + sizeof(uint8_t);
	}



	return 1;
}

// 再送パケットの取得
struct msghdr *get_resendst(int fd,struct msglist *tail,uint8_t *ack_arr){
	struct msglist *mlp = tail;
	struct msglist *mlp_next;
	int err = 0;
	char buf[BUFFERSIZE];

// タイマーでちょっと待ち
//---------------------------------	
//	int timeout = __RE__TIMEOUT;
	struct timespec ts;		
	ts.tv_sec = __RE__STIMER;
	ts.tv_nsec = __RE__NANOTIMER;
	nanosleep(&ts, NULL);
//---------------------------------

	while(1){

//		timeout--;
		memset(buf,'\0',sizeof(buf));

		err = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
/*
		if(timeout<0){
			printf("time out\n");
			break;
		}
*/
		if(err<0){
			//printf("packet wait\n");
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

//		printf("get packet id is [%u]\n", mlp_next->id);
//		printf("get packet len is [%u]\n", mlp_next->length);
//		printf("get packet key is [%u]\n", mlp_next->key);
//		printf("get packet len is [%s]\n", mlp_next->data);

		// 既に取得しているパケット
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

// チェックリストの初期化
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

// 終了パケットの生成(クライアント側と同じ)
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

	err = sendto(sl[0].sock, buf, sizeof(buf), 0, sl[0].addr, sl[0].addr_len);

	if(!err){
		printf("close err \n");
		return 0;
	}

	return 1;
}

// データストリームの再構築
int collect_datas(void *ubuf,struct msglist *head,size_t bufsize){
	struct msglist *p = head;
	struct msglist *np = head->next;
	char *b;
	int size = 0;

	memset(ubuf,'\0',bufsize);

	printf("buf size = %d\n",bufsize);

	while(np!=NULL){
		p = np;
		np = np->next;

		size = size + (int)p->length;
		if(size >= bufsize){
			printf("over flow\n");
			return 0;
		}

		b = ubuf + MSGSIZE*(p->id-1);

		//printf("write id:%u\n",p->id);
		//printf("write ln:%u\n",p->length);
		//printf("write da:%s\n",p->data);
		memcpy(b,p->data,p->length);

		//printf("[id=%u] [length=%u]\n",p->id,p->length);

	}

	return size;

}

//　受信
int recvst(int fd,void *ubuf, size_t size, unsigned int flag){
	int err = 0;
	int msgsize = 0;// all stream data size
	struct msglist *head;// first packet
	struct msglist *last;// first packet
	struct msglist *ml;
// タイマー
//-----------------------------
	int timeout = TIMEOUT;
	struct timespec ts;		
	ts.tv_sec = STIMER;
	ts.tv_nsec = NANOTIMER;
//-----------------------------

	// コネクション関数
	head = accept_recvst(fd,flag);// get connection packet
	if(!head)return -1;

	ml = head;

// 受信場所定義
//--------------------------------------------
	struct socklist recv_sl[MAXIF];
	char *dev[HOW_MANY_IF] = {IF_LIST};
	int wcount[HOW_MANY_IF] = {WORDCOUNT};

	if(HOW_MANY_IF==0){
		recv_sl[0].sock = fd;
	}else if(HOW_MANY_IF == 2){
		recv_sl[0] = set_saddr(PORT,dev[0],wcount[0]);
		recv_sl[1] = set_saddr(PORT2,dev[1],wcount[1]);
	}
//--------------------------------------------

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
	printf("max id = %d\n",ids);

	uint8_t *ack_arr;
	// uint32サイズの配列無いからメモリで配列っぽいものを作成
	if ((ack_arr = (uint8_t *) malloc(sizeof(uint8_t)*ids)) == NULL) {
		printf("malloc error \n");
		return -1;
	}

	init_ack_arr(ack_arr,ids);

//---------------------
	int if_num = 0;

	// データ部受信
	while(1){

//		err = do_recvst(fd,ml,flag,ack_arr); //通常版
		err = do_recvst(recv_sl[if_num].sock,ml,flag,ack_arr); //複数ポート版

	        if(HOW_MANY_IF!=0){
        	        if_num = (if_num+1)%HOW_MANY_IF;
        	}

     		printf("IF = %d\n",if_num);


		// 終了
		if(err == 0){
			printf("get close packet\n");
			break;
		// パケット到達なし
		}else if(err == 2){
			nanosleep(&ts, NULL);
			timeout--;
			if (timeout==0){
				break;
			}
			continue;
		// パケット到達
		}else{
			timeout = TIMEOUT;
			ml = ml->next;

			printf("[recv]:id = %u\n",ml->id);

		}

	}

	// 受信データがなければ終了
	if(!head->next) return -1;

//再送処理
//----------------------------------------------------
	long counter = 0;

	while(1){

		// 再送要求
		err = rq_resendst(&sl,ack_arr,ids);

		if(err<0){
			printf("rq_resendst err\n");
			break;
		}

		// 再送パケット受信
		ml = get_resendst(fd,ml,ack_arr);
		// チェックリストの確認
		err = check_ackarr(ack_arr,ids);

//		printf("check counter = %d\n", err);
		counter = counter + err;

		if(err==0){
			printf("get all packet:%d\n",err);
			break;
		}

	}

	// 終了
	err = close_sendst(&sl);
	//printf("close_sendst = %d\n",err );


//----------------------------------------------------

	//printf("head->id:%u\n",head->next->id);

	//再送用ソケットの開放
	close(sl.sock);

	// リストのデータのをまとめる
	msgsize = collect_datas(ubuf,head,size);

	// リストのメモリ解放
	free_msglist(head);

	// チェック配列のメモリの開放
	free(ack_arr);

//	return msgsize; 
	return counter;
}


int main(){
	int sock;
	struct sockaddr_in addr;
	//char buf[BUFFERSIZE];
	char *buf;
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

	if ((buf = (char *) malloc(sizeof(char)*BUFFERSIZE)) == NULL) {
		printf("malloc error \n");
		return 0;
	}

	//while(1){

		memset(buf, '\0', sizeof(buf));

		err = recvst(sock,buf,BUFFERSIZE,0);

//		err = recv(sock, buf, sizeof(buf), 0);

		if(err<0){
			printf("recv err\n");
		}

	printf("<<stream data>>\n%s\n", buf);
	printf("data len = %d\n",err);

	//}

	free(buf);
	close(sock);

	return 0;
}
