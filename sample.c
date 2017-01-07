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

	int k = 0;
	for(i=0;i<5;i++){
		printf("k = %d\n",k);	
		k = (k+1)%1;
	}

	return 0;
}
