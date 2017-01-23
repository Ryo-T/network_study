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

int add_msglist(struct msglist *front,void *m,int len){
	struct msglist *ml;
//	struct data_hdr *h;
	
	/* リストの記憶領域の確保 */
	if ((ml = (struct msglist *) malloc(sizeof(struct msglist))) == NULL) {
		printf("malloc error 2\n");
		return 0;
	}

	front->next = ml;
	ml->next = NULL;
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
	char buf[MSGSIZE+1];
	char *bp = msg;
	char *bp2 = bp + MSGSIZE;

	char b[MSGSIZE+1];
	memset(b, '\0', MSGSIZE+1);

	if((int)bp2-(int)msg<=len){
		
		memcpy(buf,bp,MSGSIZE);	
		err = add_msglist(head,buf,MSGSIZE);
		
		if(!err){
			printf("sendst err\n");
			return 0;
		}

		printf("1:len=%d\n",MSGSIZE);

		bp = bp2;
		bp2 = bp2 + MSGSIZE;



		memcpy(b,head->next->data,MSGSIZE);//メモリのコピー

	}else{
		
		int memlen = len-((int)bp-(int)msg);

		memcpy(buf,bp,memlen);	
		err = add_msglist(head,buf,memlen);

		if(!err){
			printf("sendst err\n");
			return 0;
		}

		printf("2:len=%d\n",memlen);

		//memset(b, '\0', MSGSIZE);

		memcpy(b,head->next->data,memlen);//メモリのコピー
			
	}

	if(!head->next->data)
		printf("NULL\n");


	printf("hdr->data = %d\n",(int)sizeof(head->next->data));
	printf("hdr->data = %s\n",head->next->data);
	printf("head = %d\n",(int)head);

	//memset(b, '*', MSGSIZE+1);
	printf("%s\n",b);

	return err;
}

void free_msglist(struct msglist *ml){
	struct msglist *p = NULL;
	struct msglist *np = ml;

	do{
		p = np;
		np = np->next;

		free(p);

		printf("free p\n");

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

	ml->next = NULL;
	ml->back = NULL;

	err = create_msglist(ml,msg,len);
	if(!err){
		printf("create_msglist err\n");
		return 0;
	}

	char b[MSGSIZE+1];
	memset(b, '\0', MSGSIZE+1);
	memcpy(b,ml->next->data,MSGSIZE);
	printf("%s\n",b);



	err = sendto(fd, msg, len, 0, addr, addr_len);
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