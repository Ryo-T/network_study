#include <stdio.h>
#include <netinet/in.h>
//#include <time.h>

struct msglist{
	struct msglist *next;
	struct msglist *back;
	uint32_t id;
	uint16_t key;
	uint16_t length;
	char data[10];
};

int main(){
	struct msglist ml;
	char buf[1024];
	uint32_t p,q;
	memset(buf,'\0',sizeof(buf));

	ml.id = 100000;

	memcpy(buf,&ml.id,sizeof(uint32_t));

	printf("%s\n",buf);
	printf("%u\n",(uint32_t)buf);
	//printf("%u\n",*buf);
	//printf("%u\n",(uint32_t)*buf);
	memcpy(&q,buf,sizeof(uint32_t));
	printf("%u\n",q);

	memcpy(&p,&ml.id,sizeof(uint32_t));
	printf("%u\n",p);

	return 0;
}
