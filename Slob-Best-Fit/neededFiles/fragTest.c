#include <stdio.h>
#include <unistd.h>

int main(){  
	unsigned long amt_claimed, amt_free;
	int i;

	for(i = 0; i < 3; i++){
		amt_claimed = syscall(360);
		amt_free= syscall(359);
		long double percent = (100 * ((long double)amt_free / (long double)amt_claimed));

		printf("|------------------------------------------\n");
		printf("| Amount  Claimed: %lu\n", amt_claimed);
		printf("| Amount  Free: %lu\n", amt_free);
		printf("| Pefcent Claimed: %Lf\n", percent);
		printf("|------------------------------------------\n\n");
		sleep(2);
	}
     return 0;
}