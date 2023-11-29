#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>	
#include <pthread.h>
#define SLEEP (174)
#define AWAKE (177)
#define AUDIT if (1)

#define THREADS (4)
#define get_id(dummy) (long) dummy



void *goto_sleep(void *dummy){
	int id = get_id(dummy);
	
	AUDIT{
		printf("Thread[%d]: ready to sleep\n", id);
		fflush(stdout);
	}

	syscall(SLEEP);

	AUDIT{
		printf("Thread[%d]: some budy wake me up\n", id);
		fflush(stdout);
	}

	pthread_exit((void *)&id);

}

void *awake(void *dummy){
	int id = get_id(dummy);
	int i;
	AUDIT{
		printf("Thread[%d]: I'll wait some time before awake\n", id);
		fflush(stdout);
	}
	sleep(4);
	AUDIT{
		printf("Thread[%d]: Ok now i can awake all\n", id);
		fflush(stdout);
	}
	syscall(AWAKE);
	

}



int main(int argc, char **argv){

	long i;
	pthread_t *tids;

	tids = malloc(sizeof(pthread_t) * (THREADS * 2));
	if (tids == NULL){
		perror("Some probelm with malloc\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < (THREADS + 1) ; i++){
		if (i < THREADS){
			if (pthread_create(&tids[i], NULL, goto_sleep, (void *)i)){
				perror("Some problem with pthread create");
				exit(EXIT_FAILURE);
			}
		}else{
			if (pthread_create(&tids[i], NULL, awake, (void *)i)){
				perror("Some problem with pthread create");
				exit(EXIT_FAILURE);
			}
		}
	}

	for (i = 0; i < (THREADS * 1); i++ ){
		if (pthread_join(tids[i], NULL)){
			perror("Some problem with pthread_join\n");
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);

}
