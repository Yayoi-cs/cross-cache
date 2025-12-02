#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "tsune"
#define IOCTL_CMD_ALLOC 0x810
#define IOCTL_CMD_FREE 0x811
#define IOCTL_CMD_READ 0x812
#define IOCTL_CMD_WRITE 0x813
#define MSG_SZ 1024
struct user_req {
    int idx;
    char *userland_buf;
};

char *ptrs[1024];
static struct kmem_cache *tsune_cache;

static long tsune_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct user_req req;
    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
        return -EFAULT;
    }
    if (req.idx < 0 || req.idx >= 1024) {
        return -EINVAL;
    }

    switch(cmd) {
        case IOCTL_CMD_ALLOC:
            ptrs[req.idx] = kmem_cache_alloc(tsune_cache, GFP_KERNEL);
            if (!ptrs[req.idx]) {
                return -ENOMEM;
            }
            break;
        case IOCTL_CMD_FREE:
            if (ptrs[req.idx]) {
                kmem_cache_free(tsune_cache, ptrs[req.idx]);
                //ptrs[req.idx] = NULL;
            }
            break;
        case IOCTL_CMD_READ:
            if (ptrs[req.idx]) {
                if (copy_to_user(req.userland_buf, ptrs[req.idx], MSG_SZ)) {
                    return -EFAULT;
                }
            }
            break;
        case IOCTL_CMD_WRITE:
            if (ptrs[req.idx]) {
                if (copy_from_user(ptrs[req.idx], req.userland_buf, MSG_SZ)) {
                    return -EFAULT;
                }
            }
            break;
        default:
            return -EINVAL;
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
