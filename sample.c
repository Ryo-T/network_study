#include <stdio.h>
//#include <time.h>
#include <sys/time.h>

#define MAX 5
#define LIST "aaa","bbb","ccc","ddd","eee"

void f(int *k){
	k[0] = 100;
	k[1] = 200;
	k[2] = 300;

	return;
}

int main(){
	struct timespec ts;
	ts.tv_sec = 1;// 秒
	ts.tv_nsec = 0;// nano秒

	char *n[MAX] = {LIST};

	int i;
	for(i = 0;i<MAX;i++){
		printf("n = %s\n",n[i]);
		nanosleep(&ts, NULL);
	}


	return 0;
}
