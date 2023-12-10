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
    element *head = malloc((sizeof(element *)));
    head->val = -1;
    head->next = NULL;
    l->head = head;
    pthread_create(&tid, NULL, house_keeper, (void *)l);
    return 0;
}

int rcu_insert(rcu_list_t *l, int val)
{
    int ret = -1;

    
    if (rcu_search(l, val, 0)){
        element *elem = malloc(sizeof(element *));
        if (elem == NULL) goto exit_insertion;
        elem->val = val;
        pthread_spin_lock(&l->lock);
        elem->next = l->head->next;
        l->head->next = elem;

        asm volatile ("mfence");

        ret = 0;
        pthread_spin_unlock(&l->lock); 
    }
exit_insertion:
    
    return ret;
}

int rcu_remove(rcu_list_t *l, int val)
{
    int current_epoch;
    int ret = -1;
    element *n = l->head->next;
    element *temp = l->head;
    pthread_spin_lock(&l->lock);

    for (; n != NULL; n = n->next){
        if (n->val == val) break;
        temp = n;
    }

/*
    if (n == NULL) goto exit_thread;

    if (n->val == val) {
        temp = n;
        goto handle_val;
    }
    for (; n->next != NULL; n = n->next){
        if (n -> next -> val == val) {
            temp = n->next;
            break;
        }
    }
        if (n == NULL || n -> next == NULL) goto exit_thread;
*/
    if (n == NULL) goto exit_thread;

    current_epoch = l->epoch_index;
    l->epoch_index = (current_epoch + 1) % 2;    
    while(l->pendings[current_epoch]);
    l->pendings[current_epoch] = 0;
    temp->next = n->next;
    free(n);

    asm volatile ("mfence");

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
