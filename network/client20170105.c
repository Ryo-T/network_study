#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>

#define MSGSIZE 11


in_addr_t inet_addr(const char *cp);
int close(int fd);

struct msglist{
	struct msglist *next;
	struct msglist *back;
	uint16_t id;
	uint16_t length;
	char data[MSGSIZE+1];
};

int add_msglist(struct msglist *front,void *m,uint16_t len,uint16_t lid){
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
	ml->length = len;

	//memset(ml->data, '\0', MSGSIZE+1);
	memcpy(ml->data,m,len);//メモリのコピー

	printf("size=%d\n",sizeof(ml->data));
	printf("data = \n%s\n",front->next->data);
	printf("p = %d\n",(int)front);

	return 1;

}


int create_msglist(struct msglist *head,void *msg,size_t len){
	int err = 0;
	uint16_t lid = 1;
	struct msglist *ml = head;
	char buf[MSGSIZE+1];
	char *bp = msg;
	char *bp2 = bp + MSGSIZE;
	char b[MSGSIZE+1];

	memset(b, '\0', MSGSIZE+1);

	do{

		if((int)bp2-(int)msg<=len){
			
			memcpy(buf,bp,MSGSIZE);	
			err = add_msglist(ml,buf,MSGSIZE,lid);
			
			if(!err){
				printf("sendst err\n");
				return 0;
			}

			printf("1:len=%d\n",MSGSIZE);

			bp = bp2;
			bp2 = bp2 + MSGSIZE;

			if(ml->next==NULL){
				printf("create list err\n");
				return 0;
			}

			ml = ml->next;
			lid ++;

			memcpy(b,head->next->data,MSGSIZE);//メモリのコピー

		}else{
			
			memset(b, '\0', MSGSIZE+1);

			int memlen = len-((int)bp-(int)msg);

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

			memcpy(b,head->next->data,memlen);//メモリのコピー
				
		}

	}while((int)bp2-(int)msg<=len);

	if(!head->next->data)
		printf("NULL\n");


	printf("hdr->data = %d\n",(int)sizeof(head->next->data));
	printf("hdr->data = %s\n",head->next->data);
	printf("head = %d\n",(int)head);

	//memset(b, '*', MSGSIZE+1);
	printf("%s\n",b);

	return err;
}

int do_sendst(int fd,struct msglist *ml, 
				struct sockaddr *addr, int addr_len){
	int err = 0;
	int hsize = sizeof(uint16_t)*2;
	int mlen = 0;
	struct msglist *p = ml;
	char buf[hsize+MSGSIZE+1];
	char *cp;

//	printf("aaa\n");


	if(!p){
		printf("do_send err 1\n");
		return 0;
	}

	while(1){

		cp = buf;

		/*
		printf("buf %d\n",buf);
		printf("cp %d\n",cp);
		printf("size %d\n",sizeof(uint16_t) );
		*/

		// create packet
		mlen = hsize + p->length;
		memset(buf,'\0',sizeof(buf));
		memcpy(cp,&p->id,sizeof(uint16_t));
		cp = cp + sizeof(uint16_t);
		memcpy(cp,&p->length,sizeof(uint16_t));
		cp = cp + sizeof(uint16_t);
		memcpy(cp,p->data,p->length);

		/*
		printf("buf:%s\n",buf);
		printf("mlen:%d\n",mlen);
		*/

		err = sendto(fd, buf, mlen, 0, addr, addr_len);

		if(!err){
			printf("do_send err 2\n");
			return 0;
		}

		if(p->next==NULL){
			printf("do_send break\n");
			break;
		}

		p = p->next;

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
	struct msglist *ml;

	/* リスト先頭の記憶領域の確保 */
	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error 1\n");
		return 0;
	}

	// init list head
	ml->next = NULL;
	ml->back = NULL;
	ml->id = 0;
	ml->length = 0;

	err = create_msglist(ml,msg,len);

	if(!err){
		printf("create_msglist err\n");
		return 0;
	}

/*
	char b[MSGSIZE+1];
	memset(b, '\0', MSGSIZE+1);
	memcpy(b,ml->next->data,MSGSIZE);
	printf("%s\n",b);
*/

	err = do_sendst(fd, ml->next, addr, addr_len);


//	err = sendto(fd, msg, len, 0, addr, addr_len);




//	printf("sizeof(buf)=%d,\n%s\n",(int)sizeof(msg),(char *)msg);
//	printf("len=%d,\n%s\n",len,(char *)msg);

	free_msglist(ml);

	return err;
}

// get file data size
int get_file_size(const char *file[]){
	fpos_t fsize = 0;

	FILE *fp = fopen(file,"rb"); 
 
	fseek(fp,0,SEEK_END); 
	fgetpos(fp,&fsize); 
 
	fclose(fp);
 
	return fsize;
}

int main(){
	int sock;
	struct sockaddr_in addr;
	struct msglist *list;
	int err = 0;
	FILE *fp;
	char *file = "./sample2.txt";
	int flen = 0;

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
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");


//------------------------------------------------------------

	err = sendst(sock,buf,sizeof(buf),0,(struct sockaddr *)&addr, sizeof(addr));

	if(!err){
		printf("sendst err\n");
		return 0;
	}


//------------------------------------------------------------

	close(sock);

	return 0;
}