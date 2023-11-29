/*
* 
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This module is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* @file virtual-to-physical-memory-mapper.c 
* @brief This is the main source for the Linux Kernel Module which implements
*       a system call that can be used to query the kernel for current mappings of virtual pages to 
*	physical frames - this service is x86_64 specific in the curent implementation
*
* @author Francesco Quaglia
*
* @date November 10, 2021
*/

#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <linux/syscalls.h>


#include "lib/include/scth.h"




MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dissan Uddin Ahmed <pc.dissan@gmail.com>");
MODULE_DESCRIPTION("details on the usage of printk");

#define MODNAME "BQS"

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);
unsigned long sleeping_thread = 0x0;
module_param(sleeping_thread, ulong, 0660);

unsigned long the_ni_syscall;

spinlock_t bqs_lock;

unsigned long new_sys_call_array[] = {0, 0};//please set to sys_vtpmo at startup
#define HACKED_ENTRIES (int)(sizeof(new_sys_call_array)/sizeof(unsigned long))
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES-1)] -1};



#define AUDIT if(1)
#define NO (0x0)
#define YES (0x1)

typedef struct bqs_node{
        struct task_struct *the_task;
        unsigned long node_pid;
        int is_awake;
        int is_hit;
        struct bqs_node *prev;
        struct bqs_node *next;
}bqs_node_t;

bqs_node_t head;
bqs_node_t tail;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(1, _goto_sleep, unsigned long, none){
#else
asmlinkage long sys_goto_sleep(void){
#endif

        bqs_node_t me;

        DECLARE_WAIT_QUEUE_HEAD(bqs_queue_list);

        
        preempt_disable();
        printk("%s before spinlock %d", MODNAME, current->pid);
        spin_lock(&bqs_lock);
        me.next = &tail;
        me.prev = tail.prev;
        tail.prev->next = &me;
        tail.prev = &me;
        me.is_hit = NO;
        spin_unlock(&bqs_lock);
        printk("%s after spinlock %d", MODNAME, current->pid);
        preempt_enable();

        AUDIT{
		printk("%s: ------------------------------\n",MODNAME);
		printk("%s: aksed to %d to sleep :D\n",MODNAME, current->pid);
	}

        me.is_awake = NO;
        atomic_inc((atomic_t *)&sleeping_thread);
#ifdef CLASSIC
        wait_event_interruptible(bqs_queue_list, (me.is_awake == YES));
#else
        me.the_task = current;
        me.node_pid = current->pid;
        wait_event_interruptible(bqs_queue_list, (me.is_awake == YES));
#endif

        AUDIT{
                printk("%s: ------------------------------\n",MODNAME);
		printk("%s: Good morning[%d]: xD\n",MODNAME, current->pid);
        }

        if (me.is_awake == NO){
                printk("%s Some error occured lol\n", MODNAME);
                return -1;
        }

        atomic_dec((atomic_t *)&sleeping_thread);
	return 0;
	
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(1, _awake, unsigned long, none){
#else
asmlinkage long sys_awake(void){
#endif

        struct task_struct *the_task;
        int the_pid = -1;
        bqs_node_t *target;
        target = &head;

        AUDIT{
		printk("%s: ------------------------------\n",MODNAME);
		printk("%s: SYSCALL_AWAKE\n",MODNAME);
	}

        preempt_disable();
        spin_lock(&bqs_lock);

        if (target == NULL){
                printk("%s:No queue found\n",MODNAME);
                spin_unlock(&bqs_lock);
                return -1;
        }
        
        while (target -> next != &tail){
                printk("%s: ------------------------------\n",MODNAME);
		printk("%s: starting waking_up\n",MODNAME);
                if (target->next->is_hit == NO){
                        the_task = target->next->the_task;
                        target->next->is_awake = YES;
                        target->next->is_hit = YES;
                        the_pid = target->next->node_pid;
                        wake_up_process(the_task);
                }

                target->next = target->next->next;
        }
        spin_unlock(&bqs_lock);
        preempt_enable();
	return 0;
	
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long sys_goto_sleep = (unsigned long) __x64_sys_goto_sleep;   
long sys_awake = (unsigned long) __x64_sys_awake;       
#else
#endif


int init_module(void) {

        int i;
        int ret;

        if (the_syscall_table == 0x0){
           printk("%s: cannot manage sys_call_table address set to 0x0\n",MODNAME);
           return -1;
        }

	AUDIT{
	   printk("%s: printk-example received sys_call_table address %px\n",MODNAME,(void*)the_syscall_table);
     	   printk("%s: initializing - hacked entries %d\n",MODNAME,HACKED_ENTRIES);
	}

	new_sys_call_array[0] = (unsigned long)sys_goto_sleep;
        new_sys_call_array[1] = (unsigned long)sys_awake;


        ret = get_entries(restore,HACKED_ENTRIES,(unsigned long*)the_syscall_table,&the_ni_syscall);

        if (ret != HACKED_ENTRIES){
                printk("%s: could not hack %d entries (just %d)\n",MODNAME,HACKED_ENTRIES,ret); 
                return -1;      
        }

        head.next = &tail;
        tail.prev = &head;

        spin_lock_init(&bqs_lock);

	unprotect_memory();

        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = (unsigned long)new_sys_call_array[i];
        }

	protect_memory();

        printk("%s: all new system-calls correctly installed on sys-call table\n",MODNAME);

        return 0;

}

void cleanup_module(void) {

        int i;
                
        printk("%s: shutting down\n",MODNAME);

	unprotect_memory();
        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = the_ni_syscall;
        }
	protect_memory();
        printk("%s: sys-call table restored to its original content\n",MODNAME);
        
}