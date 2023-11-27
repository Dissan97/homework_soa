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
#include <linux/syscalls.h>

#define MODNAME "Blocking Queue Service"

MODULE_DESCRIPTION("Implementation of a Linux kernel subsystem dealing with thread management: {sleep, awake}");

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);

unsigned long the_ni_syscall;
unsigned long new_syscall_array[] = {0x0}; // set to sys_vtpmo at startup

#define HACKED_ENTRIES (int)(sizeof(new_syscall_array) / sizeof(unsigned long))
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES - 1)] - 1};

#define AUDIT if (1)

#define NO (0x0)
#define YES ((NO ^ 0x1))

static int service_sleep = 1;
module_param(service_sleep, int, 0660);
unsigned long sleeping_threads __attribute__((aligned(8)));
module_param(sleeping_threads, ulong, 0660); 

typedef struct __thread_sleeping_info
{
    struct task_struct *task;
    int pid;
    int awake;
    int is_hit;
    struct __thread_sleeping_info *next;
    struct __thread_sleeping_info *prev;
} tsi_t;

tsi_t head = {NULL, -1, -1, -1, NULL, NULL};
tsi_t tail = {NULL, -1, -1, -1, NULL, NULL};
spinlock_t lock;


#define LIBNAME "SCTH"


#define AUDIT if(1)
#define LEVEL3_AUDIT if(0)

#define MAX_ACQUIRES 4


//stuff for sys cal table hacking
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif
#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif

unsigned long cr0;
static inline void write_cr0_forced(unsigned long val) {
    unsigned long __force_order;

    /* __asm__ __volatile__( */
    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order));
}

void protect_memory(void){
    write_cr0_forced(cr0);
}

void unprotect_memory(void){
    cr0 = read_cr0();
    write_cr0_forced(cr0 & ~X86_CR0_WP);
}



int get_entries(int * entry_ids, int num_acquires, unsigned long sys_call_table, unsigned long *sys_ni_sys_call) {

        unsigned long * p;
        unsigned long addr;
        int i,j,z,k; //stuff to discover memory contents
        int ret = 0;
	int restore[MAX_ACQUIRES] = {[0 ... (MAX_ACQUIRES-1)] -1};


        printk("%s: trying to get %d entries from the sys-call table at address %px\n",LIBNAME,num_acquires, (void*)sys_call_table);
	if(num_acquires < 1){
       		 printk("%s: less than 1 sys-call table entry requested\n",LIBNAME);
		 return -1;
	}
	if(num_acquires > MAX_ACQUIRES){
       		 printk("%s: more than %d sys-call table entries requested\n",LIBNAME, MAX_ACQUIRES);
		 return -1;
	}

	p = (unsigned long*)sys_call_table;

        j = -1;
        for (i=0; i<256; i++){
		for(z=i+1; z<256; z++){
			if(p[i] == p[z]){
				AUDIT{
                        		printk("%s: table entries %d and %d keep the same address\n",LIBNAME,i,z);
                        		printk("%s: sys_ni_syscall correctly located at %px\n",LIBNAME,(void*)p[i]);
				}
				addr = p[i];
                        	if(j < (num_acquires-1)){
				       	restore[++j] = i;
					ret++;
                        		printk("%s: acquiring table entry %d\n",LIBNAME,i);
				}
                        	if(j < (num_acquires-1)){
                        		restore[++j] = z;
					ret++;
                        		printk("%s: acquiring table entry %d\n",LIBNAME,z);
				}
				for(k=z+1;k<256 && j < (num_acquires-1); k++){
					if(p[i] == p[k]){
                        			printk("%s: acquiring table entry %d\n",LIBNAME,k);
                        			restore[++j] = k;
						ret++;
					}
				}
				if(ret == num_acquires){
					goto found_available_entries;
				}
				return -1;	
			}
                }
        }

        printk("%s: could not locate %d available entries in the sys-call table\n",LIBNAME,num_acquires);

	return -1;

found_available_entries:
	printk("%s: ret is %d\n",LIBNAME,ret);
	memcpy((char*)entry_ids,(char*)restore,ret*sizeof(int));
	*sys_ni_sys_call = addr;

	return ret;

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(1,_goto_sleep,int, unused){
#else
asmlinkage void sys_goto_sleep(void){
#endif
    printk("[%s]: SYCALL GOTO_SLEEP", MODNAME);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(1,_awake,int, unused){
#else
asmlinkage void sys_awake(void){
#endif
    printk("[%s]: SYSCALL AWAKE", MODNAME);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
long sys_goto_sleep = (unsigned long) __x64_sys_goto_sleep;
long sys_awake = (unsigned long) __x64_sys_awake;
#else
#endif

int init_module (void){
    int i;
    int ret;

    if (the_syscall_table == 0x0){
        printk("[%s]: Cannot manage syscall table addr -> 0x0\n", MODNAME);
        return -1;
    }

    AUDIT{
        printk("[%s]: Got the syscall_table_address -> %px\n", MODNAME, (void *)the_syscall_table);
        printk("[%s]: init hacked_entries=%d", MODNAME, HACKED_ENTRIES);
    }

    spin_lock_init(&lock);

    head.next = &tail;
    tail.prev = &head;

    new_syscall_array[0] = (unsigned long)sys_goto_sleep;
    new_syscall_array[1] = (unsigned long)sys_awake;

    ret = get_entries(restore, HACKED_ENTRIES, (unsigned long *) the_syscall_table, &the_ni_syscall);

    if (ret != HACKED_ENTRIES){
        printk("[%s]: could not hack %d entries could just (%d)\n", MODNAME, HACKED_ENTRIES, ret);
        return -1;
    }

    unprotect_memory();

    for (i = 0; i < HACKED_ENTRIES; i++){
        ((unsigned long*)the_syscall_table)[restore[i]] = (unsigned long) new_syscall_array[i];
    }
    protect_memory();

    printk("[%s]: all the syscall installed correctly\n", MODNAME);
    return 0;
}

void cleanup_module(void){
    int i;
    
    printk("[%s]: Shutdown mod\n", MODNAME);

    unprotect_memory();
    for (i=0; i < HACKED_ENTRIES; i++){
        ((unsigned long *)the_syscall_table)[restore[i]] = the_ni_syscall;
    }
    protect_memory();
    printk("[%s]: Origninal Syscal_table_restored\n", MODNAME);
}
