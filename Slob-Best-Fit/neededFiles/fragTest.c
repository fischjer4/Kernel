#include <stdio.h>
#include <unistd.h>

int main(){  
	unsigned long amt_claimed, amt_free;
	char *dummy1, dummy2, dummy3, dummy4,dummy5,dummy6, dummy7;

	printf("------------------------------------------\n");
	printf("|  Allocating 7 buffers of heap memory   \n");
	print("|										 \n");
	printf("|\n");
	dummy1 = (char*)malloc(100);
	dummy2 = (char*)malloc(7553);
	dummy3 = (char*)malloc(1456);
	dummy4 = (char*)malloc(894);
	dummy5 = (char*)malloc(1555);
	dummy6 = (char*)malloc(5);
	dummy7 = (char*)malloc(15729);

	amt_claimed = syscall(360);
	amt_free= syscall(359);

	printf("|------------------------------------------");
	printf("| Amount  Claimed: %lu\n", amt_claimed);
	printf("| Amount  Free: %lu\n", amt_free);
	printf("| Pefcent Claimed: %lu\n", 100 * (amt_claimed / amt_free) );
	printf("------------------------------------------\n\n");

     return 0;
}