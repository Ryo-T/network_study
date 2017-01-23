#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>


#define MSGSIZE 9000
#define KYE 100
#define SENDTOADDR "192.168.211.138"


// 送信先アドレス
#define SADDR "192.168.211.137","192.168.211.140","192.168.211.138"
// 送信先ポート
#define PORT 8000
// 再送パケット受信ポート
#define CPORT 8100

// インターフェース関連
#define MAXIF 10 // default 10
#define HOW_MANY_IF 2
//#define IF_LIST "lo0","lo0"
#define IF_LIST "ens33","ens38","ens39"
//#define WORDCOUNT 3,3
#define WORDCOUNT 5,5,5

#define SECTIMER 0//1秒
#define NANOTIMER 1000//ナノ秒

// TIMEOUT関連
#define __RE__TIMEOUT 400000
#define __RE__SECTIMER 0//1秒
#define __RE__NANOTIMER 10//ナノ秒

#define __CONNECTION__SECTIMER 0
#define __CONNECTION__NANOTIMER 1000

#define __CLOSE__SECTIMER 0
#define __CLOSE__NANOTIMER 10000

in_addr_t inet_addr(const char *cp);
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

struct socklist set_caddr(int port,char *dev,int wcount,char *addr){
        struct socklist sl;
        static struct sockaddr_in addr2;

        sl.sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sl.sock<0){
                printf("sl.sock err\n");
                return sl;
        }

        addr2.sin_family = AF_INET;
        addr2.sin_port = htons(port);
        addr2.sin_addr.s_addr = inet_addr(addr);

        sl.addr = (struct sockaddr *)&addr2;
        sl.addr_len = sizeof(addr2);

	setsockopt(sl.sock,SOL_SOCKET, SO_BINDTODEVICE, dev, wcount);

        return sl;
}


// サブソケットの生成
struct socklist set_saddr(void){
	int err = 0;
	struct socklist sl;
	static struct sockaddr_in addr2;

	sl.sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sl.sock<0){
		printf("sl.sock err\n");
		return sl;
	}

	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(CPORT);
	addr2.sin_addr.s_addr = INADDR_ANY;

 	sl.addr = (struct sockaddr *)&addr2;
 	sl.addr_len = sizeof(addr2);

 	err = bind(sl.sock, (struct sockaddr *)&addr2,sizeof(addr2));

	if(err<0){
		printf("bind err\n");
		return sl;
	}

 	return sl;
}

// メッセージリストの先頭(コネクションパケット)の作成
void init_list_head(struct msglist *ml,size_t len){

	// idの総数(送信するパケットの総数)の計算
	int ids = (len-2)/MSGSIZE + 1;
	printf("max id = %d\n",ids);


	// msglistの初期化
	ml->next = NULL;
	ml->back = NULL;
	ml->id = 0;
	ml->key = KYE;
	ml->length = sizeof(uint32_t);

	// msaglistのデータ部にidの総数を挿入
	memset(ml->data, '\0', MSGSIZE+1);
	memcpy(ml->data,&ids,sizeof(uint32_t));

	// 送信側のサブインターフェースの通知処理
	//-------------------------------------

	//-------------------------------------

	return;
}

// 分割したデータをmsglistに追加
int add_msglist(struct msglist *front,void *m,uint16_t len,uint32_t lid){
	struct msglist *ml;

	// リストの記憶領域の確保
	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error 2\n");
		return 0;
	}

	front->next = ml;
	ml->next = NULL;
	ml->id = lid;
	ml->key = 0;
	ml->length = len;

	memcpy(ml->data,m,len);//メモリのコピー

	//printf("[add_msglist]:id=%u\n",(uint32_t)ml->id);
	//printf("[add_msglist]:key=%u\n",(uint16_t)ml->key);
	//printf("[add_msglist]:size=%u\n",(uint16_t)ml->length);
	//printf("[add_msglist]:data\n%s\n",front->next->data);
	//printf("[add_msglist]:front *p = %d\n",(int)front);

	return 1;

}

// 分割したデータでmsglistを作る処理  *head にリストが全部ついてるはず
int create_msglist(struct msglist *head,void *msg,size_t len){
	int err = 0;
	uint32_t lid = 1;
	struct msglist *ml = head;
	char buf[MSGSIZE+1];
	char *bp = msg;
	char *bp2 = bp + MSGSIZE;

	memset(buf, '\0', MSGSIZE+1);

	do{

		// 分割したデータがパケットの最大サイズと同じ時の処理
		if((int)bp2-(int)msg<=len){

			memcpy(buf,bp,MSGSIZE);
			err = add_msglist(ml,buf,MSGSIZE,lid);

			if(!err){
				printf("sendst err\n");
				return 0;
			}

			//printf("[create_msglist]1:len=%d\n",len);
			//printf("[create_msglist]1:data = %s\n",buf);

			bp = bp2;
			bp2 = bp2 + MSGSIZE;

			if(ml->next==NULL){
				printf("create list err\n");
				return 0;
			}

			ml = ml->next;
			lid ++;

		// 分割したデータがパケットの最大サイズより小さい時の処理
		}else{

			memset(buf, '\0', MSGSIZE+1);

			int memlen = len-((int)bp-(int)msg)-1;

			memcpy(buf,bp,memlen);
			err = add_msglist(head,buf,memlen,lid);

			if(!err){
				printf("sendst err\n");
				return 0;
			}

			//printf("[create_msglist]2:memlen=%d\n",memlen);
			//printf("[create_msglist]2:data = %s\n",buf);


			bp = bp2;
			bp2 = bp2 + MSGSIZE;

			if(ml->next==NULL){
				printf("create list err\n");
				return 0;
			}

			ml = ml->next;
			lid ++;
		}

	}while((int)bp2-(int)msg<=len);

	if(!head->next->data)
		printf("NULL\n");

	//printf("hdr->data = %d\n",(int)sizeof(head->next->data));
	//printf("hdr->data = %s\n",head->next->data);
	//printf("head = %d\n",(int)head);

	return err;
}


// msglistからパケットの生成
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

// コネクション
int connect_sendst(struct socklist *sl,struct msglist *ml){
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;// ヘッダーサイズ
	int err = 0;
	char buf[hsize + 4 + 1];// ヘッダサイズ + 総id数(4バイト)

	if(!ml){
		printf("connection err 1\n");
		return 0;
	}

	memset(buf,'\0',sizeof(buf));
	create_packet(buf,ml);

	err = sendto(sl[0].sock, buf, sizeof(buf), 0, sl[0].addr, sl[0].addr_len);

	if(!err){
		printf("connection err 2\n");
		return 0;
	}

	return 1;

}

// メインのパケットの送信
int do_sendst(struct socklist *sl,struct msglist *ml){
	int err = 0;
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;
	int mlen = 0;
	struct msglist *p = ml;
	char buf[hsize+MSGSIZE+1];
	int if_num = 0;

	struct timespec ts;
	ts.tv_sec = SECTIMER;
	ts.tv_nsec = NANOTIMER;

	if(!p){
		printf("do_send err 1\n");
		return 0;
	}

	while(1){

		// 流すメッセージの長さを計算
		mlen = hsize + p->length;

		// パケット生成
		memset(buf,'\0',sizeof(buf));
		create_packet(buf,p);

		// id = 3と8のパケットをドロップ
//		if(p->id!=3&&p->id!=8)
		err = sendto(sl[if_num].sock, buf, mlen, 0, sl[if_num].addr, sl[if_num].addr_len);
//		printf("IF:%d,addr:%u\n",if_num,&sl[if_num].addr);

		if(!err){
			printf("do_send err 2\n");
			return 0;
		}

		if(p->next==NULL){
			printf("do_send end\n");
			break;
		}

		p = p->next;

		nanosleep(&ts, NULL);

//if関連
//---------------------------------------
		if(HOW_MANY_IF!=0){
			if_num = (if_num+1)%HOW_MANY_IF;
		}
//		printf("IF = %d\n",if_num);
//---------------------------------------

	}

	return 1;

}

// サーバープログラムと同じ
// 受信パケットからmsglist化する関数
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

//	printf("[rebuild]:id = %u\n",ml->id);
//	printf("[rebuild]:key = %u\n",ml->key);
//	printf("[rebuild]:length = %u\n",ml->length);
//	printf("[rebuild]:data = %s\n",ml->data);

	return ml;
}

// 再送
int re_sendst(struct socklist *recvsl,struct socklist *sendsl,struct msglist *ml){
	int err = 0;
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;
	uint32_t rid;
	struct timespec ts;
	struct msglist *mlp;
	struct msglist *p;
	char recvbuf[hsize+MSGSIZE+1];
	char sendbuf[hsize+MSGSIZE+1];

	int timeout = __RE__TIMEOUT;
	ts.tv_sec = __RE__SECTIMER;
	ts.tv_nsec = __RE__NANOTIMER;


	while(1){
		p = ml;
		memset(recvbuf,'\0',sizeof(recvbuf));
		memset(sendbuf,'\0',sizeof(sendbuf));

		// 再送パケットの受信
		err = recvfrom (recvsl->sock, recvbuf, sizeof(recvbuf), 
					MSG_DONTWAIT,recvsl->addr, &recvsl->addr_len);

		// 何か受信したら実行
		if(err > 0){

			mlp = rebuild_msglist(recvbuf);

			// 終了idを手に入れたら終了
			if(mlp->id==~0){
				free(mlp);
				break;

			// 再送id以外を受信した場合の終了
			}else if(mlp->id!=~0-1){
				free(mlp);
				printf("resend: no resend id\n");
				break;

			}

			memcpy(&rid,mlp->data,sizeof(uint32_t));
//			printf("request id:%u\n",(uint32_t)rid);

			while(1){
				// msglistの先頭から該当するパケットに至るまで検索
				//if((uint32_t)*mlp->data==p->id){
				if(rid==p->id){
					create_packet(sendbuf,p);
					err = sendto(sendsl[0].sock, sendbuf, sizeof(sendbuf), 0,
											 sendsl[0].addr, sendsl[0].addr_len);
					//printf("resend:err = %d\n",err);
					printf("resend:id = %u\n",p->id);
					//printf("resend:key = %u\n",p->key);
					//printf("resend:length = %u\n",p->length);
					//printf("resend:data = %s\n",p->data);

					// タイマーリセット
					timeout = __RE__TIMEOUT;

					break;
				}else if(p->next==NULL){
					printf("resend: no much id\n");
					break;
				}

				p = p->next;
			}

			free(mlp);
		}
		nanosleep(&ts, NULL);
		timeout--;

		//printf("timeout = %d\n", timeout);

		// 再送タイムアウト処理
		if(timeout==0){
			printf("resend: timeout\n");
				break;
		}

	}

	printf("end resend\n");

	return 1;

}

// コネクション終了(なくてもいい？？)
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

// msglistの開放
void free_msglist(struct msglist *ml){
	struct msglist *p = NULL;
	struct msglist *np = ml;

	do{
		p = np;
		np = np->next;

		//printf("free [id=%d]\n",p->id);

		free(p);

	}while(np!=NULL);

	return;
}

// 本体
int sendst(int fd, void *msg, size_t len, unsigned int flags,
							 struct sockaddr *addr, int addr_len){
	int err = 0;
	int i;
	char *dev[HOW_MANY_IF] = {IF_LIST};
	int wcount[HOW_MANY_IF] = {WORDCOUNT};
	char saddr[MAXIF][16] = {SADDR};
	int sport = PORT;
	struct msglist *ml;
	struct socklist sl[MAXIF];


	// リスト先頭の記憶領域の確保
	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error 1\n");
		return 0;
	}

	// 先頭のmsglistの初期化
	init_list_head(ml,len);

	err = create_msglist(ml,msg,len);
	if(!err){
		printf("create_msglist err\n");
		return 0;
	}

	// IF数を0に指定した場合はデフォルトの動作
	if(HOW_MANY_IF==0){

		sl[0].sock = fd;
		sl[0].addr = addr;
		sl[0].addr_len = addr_len;

	}else{

		// IFの数だけソケット生成
		for(i = 0; i < HOW_MANY_IF; i++){

			sl[i] = set_caddr(sport,dev[i],wcount[i],saddr[i]);
/*
			sl[i].sock = socket(AF_INET, SOCK_DGRAM, 0);
			if(!sl->sock){
				printf("sock err\n");
				return 0;
			}

			sl[i].addr = addr;
			sl[i].addr_len = addr_len;

			// MacOSだと使えないのでコメントアウト
			setsockopt(sl[i].sock, SOL_SOCKET, SO_BINDTODEVICE, dev[i], wcount[i]);
*/
			printf("sendport:%d,dev:%s,saddr:%s\n",sport,dev[i],saddr[i]);

		}

		// ちょっとずるいけど複数送信先の指定
		//----------------------------------------------
		struct sockaddr_in addr2;
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons(PORT);
		addr2.sin_addr.s_addr = inet_addr(saddr[0]);
		sl[0].addr = (struct sockaddr *)&addr2;
		sl[0].addr_len = sizeof(addr2);
		printf("saddr:%s\n",saddr[0]);

		struct sockaddr_in addr3;
                addr3.sin_family = AF_INET;
                addr3.sin_port = htons(PORT);
                addr3.sin_addr.s_addr = inet_addr(saddr[1]);
                sl[1].addr = (struct sockaddr *)&addr3;
                sl[1].addr_len = sizeof(addr3);
                printf("saddr:%s\n",saddr[1]);

		struct sockaddr_in addr4;
                addr4.sin_family = AF_INET;
                addr4.sin_port = htons(PORT);
                addr4.sin_addr.s_addr = inet_addr(saddr[2]);
                sl[2].addr = (struct sockaddr *)&addr4;
                sl[2].addr_len = sizeof(addr4);
                printf("saddr:%s\n",saddr[2]);


		//---------------------------------------------

	}

        struct timespec ts1,ts2;
        ts1.tv_sec = __CONNECTION__SECTIMER;
        ts1.tv_nsec = __CONNECTION__NANOTIMER;
	ts2.tv_sec = __CLOSE__SECTIMER;
	ts2.tv_nsec = __CLOSE__NANOTIMER;



	struct socklist rsl;// 受信用ソケットリスト

	// コネクション開始
	err = connect_sendst(sl, ml);
	if(!err)return err;

	nanosleep(&ts1,NULL);

	// 送信開始
	err = do_sendst(sl,ml->next);
	if(!err)return err;

	nanosleep(&ts2,NULL);

	// 送信終了
	err = close_sendst(sl);
	if(!err)return err;

	// 再送
	rsl = set_saddr();
	err = re_sendst(&rsl,sl,ml->next);
	if(!err)return err;


	close(sl[0].sock);

	for(i=1;i<HOW_MANY_IF;i++){
		close(sl[i].sock);
	}

//	close(sl);
	close(rsl.sock);


	free_msglist(ml);

	return err;
}

// ファイルのデータサイズを取得
long get_file_size(const char *file[]){
	fpos_t fsize;
	long sz = 0;

	FILE *fp = fopen(file,"rb");

	fseek(fp,0,SEEK_END);
	sz = ftell(fp);

	printf("FILESIZE = %lo\n",sz);

	fclose(fp);

	return sz;
}

int main(){
	int sock;
	struct sockaddr_in addr;
	struct msglist *list;
	int err = 0;
	FILE *fp;
	char *file = "./sample2.txt";
	long flen = 0;
	struct timeval s, e;



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

//	printf("buf=%d,\n%s\n",sizeof(buf),buf);
//---------------------------------------


	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(!sock){
		printf("sock err\n");
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(SENDTOADDR);

	// start timer
	gettimeofday(&s, NULL);
	err = sendst(sock,buf,sizeof(buf),0,(struct sockaddr *)&addr, sizeof(addr));
	//end timer
	gettimeofday(&e, NULL);

	printf("time = %lf\n", (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6);

	if(!err){
		printf("sendst err\n");
		return 0;
	}

	return 0;
}
