/*
 * Executable page tracing.
 *
 * Copyright (C) 2016 Suchakra Sharma
 */

#include <linux/module.h>
#include <linux/flowjit.h>
#include <linux/sched.h>

#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#ifdef CONFIG_TRACING_EXEC_PAGE

struct flowjit_event fevent;

int flowjit_enabled(void)
{
	return current->flowjit_trace;
}

void flowjit_set_enable(bool state)
{
	current->flowjit_trace = state;
}

unsigned long flowjit_arm(unsigned long flags)
{
	return flags &= ~(VM_EXEC);
}

void flowjit_disarm(struct vm_area_struct *vma)
{
	int dirty_acc = 0;
	unsigned long nflags = (vma->vm_flags | 0x000004);
	vma->vm_flags = nflags;
	dirty_acc = vma_wants_writenotify(vma);
	vma_set_page_prot(vma);
	change_protection(vma, vma->vm_start, vma->vm_end, vma->vm_page_prot, dirty_acc, 0);
}

void flowjit_build_event(struct vm_area_struct *vma)
{
	struct flowjit_event *ev;
	ev = (struct flowjit_event*) kmalloc(sizeof(*ev), GFP_KERNEL);
	ev->ts = ktime_get();
	ev->ip = vma->vm_start;
	copy_from_user(ev->buf, (void __user *) vma->vm_start, PAGE_SIZE);
	INIT_LIST_HEAD(&ev->list);
	list_add_tail(&(ev->list), &(fevent.list));
}

void flowjit_free(void)
{
	struct flowjit_event *ev, *tmp;
	list_for_each_entry_safe(ev, tmp, &fevent.list, list) {
		list_del(&ev->list);
		kfree(ev);
	}
}

//FIXME We have to enable it when we write a proper test case instead of using cat
static int flowjit_close(struct inode *inode, struct file *file)
{
	//flowjit_free();
	return 0;
}

static int flowjit_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

// FIXME For now, we just print latest entry. Make it more elegant later
static ssize_t flowjit_read(struct file *filp, char __user * buffer,
		size_t count, loff_t * ppos)
{
	struct flowjit_event *ev;

	ev = list_last_entry(&fevent.list, struct flowjit_event, list);

	if (*ppos >= sizeof(struct flowjit_event))
		return 0;
	if (*ppos + count > sizeof(struct flowjit_event))
		count = sizeof(struct flowjit_event) - *ppos;

	if (copy_to_user(buffer, ev + *ppos, count))
		return -EFAULT;

	*ppos += count;

	return count;
}


static
long flowjit_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
	case FLOWJIT_ENABLE:
		flowjit_set_enable(true);
		break;
	case FLOWJIT_DISABLE:
		flowjit_set_enable(false);
		break;
	case FLOWJIT_STATUS:
		return flowjit_enabled();
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

static struct file_operations flowjit_fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = flowjit_ioctl,
#ifdef CONFIG_COMPAT
		.compat_ioctl = flowjit_ioctl,
#endif
		.open = flowjit_open,
		.read = flowjit_read,
//		.release = flowjit_close,
};

static int __init flowjit_init(void)
{
	struct dentry *dentry;
	dentry = debugfs_create_file(FLOWJIT_DEBUGFS, S_IRUGO, NULL, NULL, &flowjit_fops);
	if (!dentry)
		pr_warning("Failed to create flowjit debugfs file\n");

	INIT_LIST_HEAD(&fevent.list);


	return 0;
}
late_initcall(flowjit_init);

static void __exit flowjit_exit(void)
{
	flowjit_free();
	return;
}

#else

int flowjit_enabled() { return false; }

#endif /* CONFIG_TRACING_EXEC_PAGE */
