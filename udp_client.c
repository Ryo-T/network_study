#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>

#define SENDTOADDR "127.0.0.1"
#define PORT 8000
#define CPORT 8100

#define MSGSIZE 110
#define KYE 100

#define MAXIF 10 // default 10
#define HOW_MANY_IF 0
//#define IF_LIST "lo0"
#define IF_LIST "ens38","ens33"
//#define WORDCOUNT 3
#define WORDCOUNT 5,5

#define TIMER 0//1秒
#define NANOTIMER 10//ナノ秒

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

/*
struct connection_hdr{
	uint16_t key;
};
*/

/*
void add_socklist(struct socklist *sl){
	struct sockaddr_in addr[HOW_MANY_IF];
	int i;

	for(i = 0; i < HOW_MANY_IF; i++){

		sl[i].sock = socket(AF_INET, SOCK_DGRAM, 0);
		if(!sl->sock){
			printf("sock err\n");
			return;
		}

		addr[i].sin_family = AF_INET;
		addr[i].sin_port = htons(PORT);
		addr[i].sin_addr.s_addr = inet_addr(SENDTOADDR);

		sl[i].addr = (struct sockaddr *)&addr;
		sl[i].addr_len = sizeof(addr[i]);

		printf("addr %u\n",inet_addr(SENDTOADDR));

	}


	return;
}
*/
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

void init_list_head(struct msglist *ml,size_t len){

	int ids = (len-2)/MSGSIZE + 1;
	printf("ids = %d\n",ids);


	// init list head
	ml->next = NULL;
	ml->back = NULL;
	ml->id = 0;
	ml->key = KYE;
	ml->length = sizeof(uint32_t);
//	ml->length = 0;

	memset(ml->data, '\0', MSGSIZE+1);
//	struct connection_hdr chdr;
//	chdr.key = KYE;
//	memcpy(ml->data,&chdr.key,sizeof(uint16_t));
	memcpy(ml->data,&ids,sizeof(uint32_t));

	return;
}

int add_msglist(struct msglist *front,void *m,uint16_t len,uint32_t lid){
	struct msglist *ml;
//	struct data_hdr *h;

	/* リストの記憶領域の確保 */
	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error 2\n");
		return 0;
	}

	front->next = ml;
	ml->next = NULL;
	ml->id = lid;
	ml->key = 0;
	ml->length = len;

	//memset(ml->data, '\0', MSGSIZE+1);
	memcpy(ml->data,m,len);//メモリのコピー

	printf("id=%u\n",(uint32_t)ml->id);
	printf("key=%u\n",(uint16_t)ml->key);
	printf("size=%u\n",(uint16_t)ml->length);
	printf("*****data*****\n%s\n",front->next->data);
	printf("front *p = %d\n",(int)front);

	return 1;

}


int create_msglist(struct msglist *head,void *msg,size_t len){
	int err = 0;
	uint32_t lid = 1;
	struct msglist *ml = head;
	char buf[MSGSIZE+1];
	char *bp = msg;
	char *bp2 = bp + MSGSIZE;
//	char b[MSGSIZE+1];

	memset(buf, '\0', MSGSIZE+1);

	do{

		if((int)bp2-(int)msg<=len){

			memcpy(buf,bp,MSGSIZE);
			err = add_msglist(ml,buf,MSGSIZE,lid);

			if(!err){
				printf("sendst err\n");
				return 0;
			}

			printf("1:len=%d\n",len);
			printf("1:data = %s\n",buf);
			printf("1:len = %d\n",(int)bp2-(int)msg);

			bp = bp2;
			bp2 = bp2 + MSGSIZE;

			if(ml->next==NULL){
				printf("create list err\n");
				return 0;
			}

			ml = ml->next;
			lid ++;

			//memcpy(b,head->next->data,MSGSIZE);//メモリのコピー

		}else{

			memset(buf, '\0', MSGSIZE+1);

			int memlen = len-((int)bp-(int)msg)-1;

			memcpy(buf,bp,memlen);
			err = add_msglist(head,buf,memlen,lid);

			if(!err){
				printf("sendst err\n");
				return 0;
			}

			printf("2:len=%d\n",memlen);


			bp = bp2;
			bp2 = bp2 + MSGSIZE;

			if(ml->next==NULL){
				printf("create list err\n");
				return 0;
			}

			ml = ml->next;
			lid ++;

			//memset(b, '\0', MSGSIZE);

			//memcpy(b,head->next->data,memlen);//メモリのコピー

		}

	}while((int)bp2-(int)msg<=len);

	if(!head->next->data)
		printf("NULL\n");


	//printf("hdr->data = %d\n",(int)sizeof(head->next->data));
	//printf("hdr->data = %s\n",head->next->data);
	//printf("head = %d\n",(int)head);

	//memset(b, '*', MSGSIZE+1);
	//printf("%s\n",b);

	return err;
}



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

int connect_sendst(struct socklist *sl,struct msglist *ml){
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;// header size
//	int chsize = sizeof(uint16_t);// connection header size
	int err = 0;
//	char buf[hsize + chsize + 1];
	char buf[hsize + 1];
//	char *cp = buf;

	if(!ml){
		printf("connection err 1\n");
		return 0;
	}



	memset(buf,'\0',sizeof(buf));
	create_packet(buf,ml);

	/*
	memcpy(cp,&ml->id,sizeof(uint16_t));
	cp = cp + sizeof(uint16_t);
	memcpy(cp,&ml->length,sizeof(uint16_t));
	cp = cp + sizeof(uint16_t);
	memcpy(cp,ml->data,connection_hdr_size);
	*/

	err = sendto(sl[0].sock, buf, sizeof(buf), 0, sl[0].addr, sl[0].addr_len);

	if(!err){
		printf("connection err 2\n");
		return 0;
	}

	return 1;

}

int do_sendst(struct socklist *sl,struct msglist *ml){
	int err = 0;
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;
	int mlen = 0;
	struct msglist *p = ml;
	char buf[hsize+MSGSIZE+1];
	int if_num = 0;

//	printf("aaa\n");


	if(!p){
		printf("do_send err 1\n");
		return 0;
	}

	while(1){

		/*
		printf("buf %d\n",buf);
		printf("cp %d\n",cp);
		printf("size %d\n",sizeof(uint16_t) );
		*/

		// create packet
		mlen = hsize + p->length;

		memset(buf,'\0',sizeof(buf));
		create_packet(buf,p);
		/*
		memset(buf,'\0',sizeof(buf));
		memcpy(cp,&p->id,sizeof(uint16_t));
		cp = cp + sizeof(uint16_t);
		memcpy(cp,&p->length,sizeof(uint16_t));
		cp = cp + sizeof(uint16_t);
		memcpy(cp,p->data,p->length);
		*/

		/*
		printf("buf:%s\n",buf);
		printf("mlen:%d\n",mlen);
		*/

		// id = 3のパケットをドロップ
		if(p->id!=3&&p->id!=8)
		err = sendto(sl[if_num].sock, buf, mlen, 0, sl[if_num].addr, sl[if_num].addr_len);

		if(!err){
			printf("do_send err 2\n");
			return 0;
		}

		if(p->next==NULL){
			printf("do_send break\n");
			break;
		}

		p = p->next;

//if関連
//---------------------------------------
		if(HOW_MANY_IF!=0){
			if_num = (if_num+1)%HOW_MANY_IF;
		}
		printf("IF = %d\n",if_num);
//---------------------------------------

	}

	return 1;

}

// サーバープログラムと同じ
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

int re_sendst(struct socklist *recvsl,struct socklist *sendsl,struct msglist *ml){
	int err = 0;
	int hsize = sizeof(uint32_t)+sizeof(uint16_t)*2;
	struct timespec ts;
	struct msglist *mlp;
	struct msglist *p;
	char recvbuf[MSGSIZE+1];
	char sendbuf[hsize+MSGSIZE+1];

	ts.tv_sec = TIMER;
	ts.tv_nsec = NANOTIMER;

	while(1){
		p = ml;
		memset(recvbuf,'\0',sizeof(recvbuf));
		memset(sendbuf,'*',sizeof(sendbuf));

		err = recvfrom (recvsl->sock, recvbuf, sizeof(recvbuf), 
					MSG_DONTWAIT,recvsl->addr, &recvsl->addr_len);

		if(err>0){
			mlp = rebuild_msglist(recvbuf);

			if(mlp->id==~0){
				free(mlp);
				break;
			}

			while(1){
				if((uint32_t)*mlp->data==p->id){
					create_packet(sendbuf,p);
					err = sendto(sendsl[0].sock, sendbuf, sizeof(sendbuf), 0,
											 sendsl[0].addr, sendsl[0].addr_len);
					printf("Re:sendto = %d\n",err);
					printf("re:id = %u\n",p->id);
					printf("re:key = %u\n",p->key);
					printf("re:length = %u\n",p->length);
					printf("re:data = %s\n",p->data);

					break;
				}else if(p->next==NULL){
					break;
				}
				printf("Re::id=%d\n",p->id);
				p = p->next;
			}

			free(mlp);
		}
		nanosleep(&ts, NULL);

	}

	printf("end resend\n");

	return 1;

}

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

int sendst(int fd, void *msg, size_t len, unsigned int flags,
							 struct sockaddr *addr, int addr_len){
	int err = 0;
	char *dev[HOW_MANY_IF] = {IF_LIST};
	int wcount[HOW_MANY_IF] = {WORDCOUNT};
	struct msglist *ml;
	struct socklist sl[MAXIF];


	/* リスト先頭の記憶領域の確保 */
	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error 1\n");
		return 0;
	}

	init_list_head(ml,len);

	printf("key = %u\n",ml->key);

	err = create_msglist(ml,msg,len);
	if(!err){
		printf("create_msglist err\n");
		return 0;
	}


	if(HOW_MANY_IF==0){

		sl[0].sock = fd;
		sl[0].addr = addr;
		sl[0].addr_len = addr_len;

	}else{

//		add_socklist(sl);

		int i;

		for(i = 0; i < HOW_MANY_IF; i++){

			sl[i].sock = socket(AF_INET, SOCK_DGRAM, 0);
			if(!sl->sock){
				printf("sock err\n");
				return 0;
			}

			sl[i].addr = addr;
			sl[i].addr_len = addr_len;


			// MacOSだと使えないのでコメントアウト
			setsockopt(sl[i].sock, SOL_SOCKET, SO_BINDTODEVICE, dev[i], wcount[i]);

			printf("dev:%s size:%u\n",dev[i],wcount[i]);

		}

	}


	struct socklist rsl;// 受信用ソケットリスト

	// コネクション開始
//	err = connect_sendst(sl[0].sock, ml, sl[0].addr, sl[0].addr_len);
	err = connect_sendst(sl, ml);
	if(!err)return err;

	// 送信開始
	err = do_sendst(sl,ml->next);
	if(!err)return err;

	// 送信終了
	err = close_sendst(sl);
	if(!err)return err;

	// 再送
	rsl = set_saddr();
	err = re_sendst(&rsl,sl,ml->next);
	if(!err)return err;

	// 最後closeパケット送らなくても大丈夫そう(?)
//	err = close_sendst(sl);
//	if(!err)return err;

/*
	err = connect_sendst(fd, ml, addr, addr_len);
	if(!err)return err;

	err = do_sendst(fd, ml->next, addr, addr_len);
	if(!err)return err;

	err = close_sendst(fd, addr, addr_len);
	if(!err)return err;
*/

//	err = sendto(fd, msg, len, 0, addr, addr_len);


//	printf("sizeof(buf)=%d,\n%s\n",(int)sizeof(msg),(char *)msg);
//	printf("len=%d,\n%s\n",len,(char *)msg);

	close(sl[0].sock);
	close(sl);

	free_msglist(ml);

	return err;
}

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
	int sock;
	struct sockaddr_in addr;
	struct msglist *list;
	int err = 0;
	FILE *fp;
	char *file = "./sample2.txt";
	long flen = 0;


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

       printf("debug\n");

	err = sendst(sock,buf,sizeof(buf),0,(struct sockaddr *)&addr, sizeof(addr));

	if(!err){
		printf("sendst err\n");
		return 0;
	}


	close(sock);

	return 0;
}
