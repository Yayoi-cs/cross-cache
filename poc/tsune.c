#include <asm-generic/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/mm.h>

#define DEVICE_NAME "tsune"
#define IOCTL_CMD_POC 0x810
#define MSG_SZ 128

static struct kmem_cache *tsune_cache;

// /sys/kernel/slab/tsune_cache/
struct user_req {
    unsigned int cpu_partial;
    unsigned int objs_per_slab;
    unsigned int object_size;
};

static long tsune_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct user_req req;
    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) { return -EFAULT; }
    if (cmd != IOCTL_CMD_POC) { return -EINVAL; }

    printk(KERN_INFO "tsune: PoC Invoked\n");
    printk(KERN_INFO "tsune: cpu_partial=%u, objs_per_slab=%u\n", req.cpu_partial, req.objs_per_slab);
    unsigned int total = 
        (req.cpu_partial + 1) * req.objs_per_slab 
        + (req.objs_per_slab - 1)
        + 1
        + (req.objs_per_slab + 1);
    printk(KERN_INFO "tsune: total: %u\n",total);
    unsigned long *list = kmalloc(sizeof(unsigned long) * total, GFP_KERNEL);
    unsigned long *head = list;
    printk(KERN_INFO "tsune: 1. allocate (cpu_partial+1)*objs_per_slab\n");
    for (int i = 0; i < (req.cpu_partial + 1) * req.objs_per_slab; i++) {
        *list++ = (unsigned long)kmem_cache_alloc(tsune_cache, GFP_KERNEL);
    }
    
    printk(KERN_INFO "tsune: 2. allocate objs_per_slab-1\n");
    for (int i = 0; i < req.objs_per_slab - 1; i++) {
        *list++ = (unsigned long)kmem_cache_alloc(tsune_cache, GFP_KERNEL);
    }
    
    printk(KERN_INFO "tsune: 3. allocate uaf object\n");
    unsigned long *uaf_obj = list;
    *list++ = (unsigned long)kmem_cache_alloc(tsune_cache, GFP_KERNEL);

    printk(KERN_INFO "tsune: 4. allocate objs_per_slab+1\n");
    for (int i = 0; i < req.objs_per_slab + 1; i++) {
        *list++ = (unsigned long)kmem_cache_alloc(tsune_cache, GFP_KERNEL);
    }

    printk(KERN_INFO "tsune: 5. free uaf object\n");
    kmem_cache_free(tsune_cache, (void *)(*uaf_obj));

    printk(KERN_INFO "tsune: 6. make page which has a uaf object empty\n");
    for (int i = 1; i < req.objs_per_slab; i++) {
        kmem_cache_free(tsune_cache, (void *)(uaf_obj[i]));
        kmem_cache_free(tsune_cache, (void *)(uaf_obj[-i]));
    }

    printk(KERN_INFO "tsune: 7. free one object per page\n");
    for (int i = 0; i < (req.cpu_partial+1) * req.objs_per_slab; i += req.objs_per_slab) {
        kmem_cache_free(tsune_cache, (void *)(head[i]));
    }

    printk(KERN_INFO "tsune: uaf object: %lx\n", *uaf_obj);
    unsigned long uaf_page = *uaf_obj & (~0xfff);
    unsigned order = get_order(req.object_size * req.objs_per_slab);
    printk(KERN_INFO "tsune: uaf page: %lx\n", uaf_page);
    printk(KERN_INFO "tsune: uaf page order: %u\n", order);
    void *new_page = alloc_pages(GFP_KERNEL, order);
    void *new_page_ptr = page_address((struct page *)new_page);
    printk(KERN_INFO "tsune: new page: %lx\n", (unsigned long)new_page_ptr);
    if ((unsigned long)new_page_ptr == uaf_page) {
        printk(KERN_INFO "tsune: cross-cache succeed!\n");
    } else {
        printk(KERN_INFO "tsune: cross-cache failed!\n");
    }
    return 0;
}

static struct file_operations tsune_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = tsune_ioctl,
};

static struct miscdevice tsune_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &tsune_fops,
};

static int __init tsune_init(void) {
    if (misc_register(&tsune_device)) { return -ENODEV; }
    tsune_cache = kmem_cache_create("tsune_cache", MSG_SZ, 0, SLAB_HWCACHE_ALIGN, NULL);
    if (!tsune_cache) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit tsune_exit(void) {
    kmem_cache_destroy(tsune_cache);
    misc_deregister(&tsune_device);
}

module_init(tsune_init);
module_exit(tsune_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tsune");
MODULE_DESCRIPTION("load to kernel heap master");
