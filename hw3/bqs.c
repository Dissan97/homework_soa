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

#define MODNAME 'BQS'

MODULE_LICENCE("GPL")
MODULE_AUTHOR("Dissan Uddin Ahmed <pc.dissan@gmail.com>")
MODULE_DESCRIPTION("Implementation of a Linux kernel subsystem dealing with thread management: {sleep, awake}")

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);

unsigned long the_ni_syscall;
unsigned long new_syscall_array[] = {0x0}; // set to sys_vtpmo at startup

#define HACKED_ENTRIES (int)(sizeof(new_syscall_array) / sizeof(unsigned long))
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES - 1)] - 1};

#define AUDIT if (0)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
#define goto_sleep(void){\
}
#else
#define goto_sleep(void){\
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
#define awake(void){\
}
#else
#define awake(void){\
}

static int bqs_init{

}

static void bqs_exit(void){

}

module_init(bqs_init)
module_exit(bqs_exit)
MODULE_LICENSE("GPL");