#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


#define THREADS (4)
#define GOTO_SLEEP (134)
#define AWAKE (174)

#define AUDIT if (1)

void *worker(void *dummy);

int main(){

    long i;
    pthread_t *tids;

    if ((tids = malloc(sizeof(pthread_t) * THREADS)) == NULL){
        perror("Cannot use malloc function");
        exit(EXIT_FAILURE);
    }

    for (i =0; i < THREADS; i++){
        if (pthread_create(&tids[i], NULL, worker, (void *) i)){
            perror("Cannot create a thead");
            exit(EXIT_FAILURE);
        }
    }

    sleep(4);

    sycall(AWAKE);
    

    for ( i = 0; i < THREADS; i++)
    {
        if (pthread_join(tids[i], NULL)){
            perror("Something wrong with pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    AUDIT
    printf("Main threed done\n");
    
    return EXIT_SUCCESS;
}

void *worker(void *dummy){
    int id = (long) dummy;
    AUDIT
    printf("Thread[%d]: I'M alive ready to go to sleep\n", id);
    syscall(GOTO_SLEEP);
    AUDIT
    printf("Thread[%d]: Now i'm awake\n", id);
    pthread_exit((void *)AWAKE);
}