#include "include/rcu_list.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
void *house_keeper(void *dummy);

int rcu_init(rcu_list_t *l)
{
    pthread_t tid;
    pthread_spin_init(&l->lock, 0);
    l->pendings = malloc(sizeof(int *) * 2);
    l->epoch_index = 0;
    pthread_create(&tid, NULL, house_keeper, (void *)l);
    return 0;
}

int rcu_insert(rcu_list_t *l, int val)
{
    int ret = -1;

    
    if (rcu_search(l, val, 0)){
        element *elem = malloc(sizeof(element *));
        elem->val = val;
        elem->next = l->head;
        pthread_spin_lock(&l->lock);
        l->head = elem;
        ret = 0;
        pthread_spin_unlock(&l->lock);
        if (l->head->next != NULL)
        printf("l->next->val=%d\n", l->head->next->val);
    }
    
    return ret;
}

int rcu_remove(rcu_list_t *l, int val)
{
    int current_epoch;
    int ret = -1;
    element *n;
    element *temp;
    pthread_spin_lock(&l->lock);
    n = l->head;
    if (n->val == val) {
        temp = n;
        goto handle_val;
    }
    for (; n->next != NULL; n = n->next){
        if (n -> next -> val == val) {
            temp = n->next;
            n -> next = temp->next;   
            break;
        }
    }

    if (n -> next == NULL) goto exit_thread;
handle_val:
    current_epoch = l->epoch_index;
    l->epoch_index = (current_epoch + 1) % 2;    
    while(l->pendings[current_epoch]);
    l->pendings[current_epoch] = 0;
    free(temp);
    ret = 0;
exit_thread:
    pthread_spin_unlock(&l->lock);
    return ret;
}

int rcu_search(rcu_list_t *l, int val, int id)
{
syscall(__NR_gettid);
    int my_epoch = l->epoch_index;
    int ret = -1;
    __sync_fetch_and_add(&l->pendings[my_epoch], 1);
    
    for (element * n= l->head; n != NULL; n = n->next){
        
        if (n->val == val) {
            
            ret = 0; 
            break;
        }
    }

    __sync_fetch_and_add(&l->pendings[my_epoch], -1);
    return ret;
}

void *house_keeper(void *dummy)
{
    rcu_list_t *l = (rcu_list_t *)dummy;
    int current_epoch;
    int next_epoch;

    while (1){
        sleep(GRACE_PERIOD);
        pthread_spin_lock(&l->lock);
        current_epoch = l->epoch_index;
        l->epoch_index = (current_epoch + 1) % 2;
        while(l->pendings[current_epoch]);
        l->pendings[current_epoch] = 0;
        pthread_spin_unlock(&l->lock);
    }
    return NULL;
}
