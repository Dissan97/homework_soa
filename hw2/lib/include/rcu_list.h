#pragma once

#include <pthread.h>

#define AUDIT if (1)
#define LOG if (1)
#define GRACE_PERIOD 0x1
typedef struct __element{
    struct __element *next;
    int val;
}element;


typedef struct __rcu_list
{
    element *head;
    unsigned long *pendings;
    unsigned int epoch_index;
    pthread_spinlock_t lock;
}rcu_list_t;

int rcu_init(rcu_list_t *l);
int rcu_insert(rcu_list_t *l, int val);
int rcu_remove(rcu_list_t *l, int val);
int rcu_search(rcu_list_t *l, int val, int id);

