/*
 * Executable page tracing.
 *
 * Copyright (C) 2016 Suchakra Sharma
 */

#ifndef _LINUX_FLOWJIT_H_
#define _LINUX_FLOWJIT_H_

#include <linux/sched.h>
#include <linux/types.h>
#include <linux/list.h>

#define FLOWJIT_DEBUGFS "flowjit"

#define FLOWJIT_ENABLE		_IO(0xF6, 0x92)
#define FLOWJIT_DISABLE		_IO(0xF6, 0x93)
#define FLOWJIT_STATUS		_IO(0xF6, 0x94)

// FIXME: support other page size
struct flowjit_event {
	u32 id;
	u64 ip;
	ktime_t ts;
	char buf[PAGE_SIZE];
	struct list_head list;
};

int flowjit_enabled(void);
unsigned long flowjit_arm(unsigned long);
void flowjit_disarm(struct vm_area_struct*);
void flowjit_set_enable(bool);
void flowjit_build_event(struct vm_area_struct*);
void flowjit_free(void);


#endif /* _LINUX_FLOWJIT_H_ */
