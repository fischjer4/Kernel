#include <stdio.h>
#include <unistd.h>

int main(){  
	unsigned long amt_claimed, amt_free;
	int i;

	for(i = 0; i < 5; i++){
		amt_claimed = syscall(360);
		amt_free= syscall(359);

		printf("|------------------------------------------\n");
		printf("| Amount  Claimed: %lu\n", amt_claimed);
		printf("| Amount  Free: %lu\n", amt_free);
		printf("| Pefcent Claimed: %f\n", (100 * (amt_claimed / amt_free)) );
		printf("|------------------------------------------\n\n");
		sleep(5);
	}
     return 0;
}