#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define PORT 8000


#define BUFFERSIZE 1024*1024
#define MSGSIZE 1000


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

/*
struct connection_hdr{
	uint16_t key;
};
*/

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

int do_recvst(int fd,struct msglist *front,unsigned int flag){
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

	front->next = ml;

	return 1;

}

int collect_datas(void *ubuf,struct msglist *head,size_t bufsize){
	struct msglist *p = head;
	struct msglist *np = head->next;
	char *b = ubuf;
	int size = 0;

	memset(ubuf,'\0',sizeof(ubuf));

	do{
		p = np;
		np = np->next;

		size = size + (int)p->length;
		if(size >= bufsize){
			printf("over flow\n");
			return 0;
		}

		memcpy(b,p->data,p->length);
		b = b + p->length;

		printf("[id=%u] [length=%u] data:%s\n",p->id,p->length,p->data);

	}while(np!=NULL);

	return size;

}

int recvst(int fd, void *ubuf, size_t size, unsigned int flag){
	int err = 0;
	int msgsize = 0;
	struct msglist *head;
	struct msglist *ml;

	head = accept_recvst(fd,flag);
	if(!head)return -1;

	ml = head;

	while(1){

		err = do_recvst(fd,ml,flag);
		if(!err) break;

		ml = ml->next;

	}

	if(!head->next) return -1;

	msgsize = collect_datas(ubuf,head,size);


	free_msglist(head);

	return msgsize; 
}


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



	while(1){

		memset(buf, '\0', sizeof(buf));

		err = recvst(sock,buf,sizeof(buf),0);

//		err = recv(sock, buf, sizeof(buf), 0);

		if(err<0){
			printf("recv err\n");
		}

	printf("<<stream data>>\n%s\n", buf);
	printf("data len = %d\n",err);

	}


	close(sock);

	return 0;
}