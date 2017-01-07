#include <stdio.h>

#define MAX 5
#define LIST "aaa","bbb","ccc","ddd","eee"

void f(int *k){
	k[0] = 100;
	k[1] = 200;
	k[2] = 300;

	return;
}

int main(){
	char *n[MAX] = {LIST};

	int i;
	for(i = 0;i<MAX;i++){
		printf("n = %s\n",n[i]);
	}

	int k[3];
	f(k);
	printf("%d,%d,%d\n",k[0],k[1],k[2]);

	return 0;
}