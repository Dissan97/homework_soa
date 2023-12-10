#include "lib/include/rcu_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define WORKERS (5)
#define OPERATIONS 100

#define set_alligned_buffer(type, name, size)\
    type __attribute__((aligned(size))) name[size]

void *worker(void *dummy);

rcu_list_t list;

int main(){
    pthread_t *tids;
    long i;

    rcu_init(&list);

    if (!(tids = malloc(sizeof(pthread_t *) * WORKERS))){
        perror("main.malloc(tids)");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < WORKERS; i++){
        pthread_create(&tids[i], NULL, worker, (void *) i);
    }


    for (i = 0; i < WORKERS; i++){
        pthread_join(tids[i], NULL);
    }

    return 0;


}

void *worker(void *dummy)
{

    int id = (long) dummy;
    int i;
    if (!id){
        AUDIT{
            printf("Thread_w[%d]: ready\n", id);
            fflush(stdout);
        }

        for (i = 0; i < (OPERATIONS ); i++){
            
            AUDIT{
                printf("Thread_w[%d] insertion of %d: %s\n",id,i,  (rcu_insert(&list, i % OPERATIONS) ? "fail":"success"));
                fflush(stdout);
            }

            

        }

        sleep(1);
        for (i = 0; i < (OPERATIONS ); i++){
            AUDIT{
                printf("Thread_w[%d] remove of %d: %s\n",id,i,  (rcu_remove(&list, i % OPERATIONS) ? "fail":"success"));
                fflush(stdout);
            }
        }
        goto exit_thread;
    }

    sleep(1);
    for (i = 0; i < OPERATIONS * 2; i++){
        
        AUDIT{
            printf("Thread_r[%d] search of %d: %s\n",id, (i % OPERATIONS),  (rcu_search(&list, (i % OPERATIONS), id) ? "fail":"success"));
            fflush(stdout);
        }
    }

exit_thread:
    return NULL;
}
