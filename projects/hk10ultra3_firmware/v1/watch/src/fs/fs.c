#include "./fs.h"

#define FS_ROOT "root"
#define FS_NAME "elm"

int mnt_init(void)
{
    register_mtd_device(FS_REGION_START_ADDR, FS_REGION_SIZE, FS_ROOT);

    if (dfs_mount(FS_ROOT, "/", FS_NAME, 0, 0) == 0)
        rt_kprintf("mount fs on flash to root success\n");
    else
    {
        rt_kprintf("mount fs on flash to root fail\n");
        if (dfs_mkfs(FS_NAME, FS_ROOT) == 0)
        {
            rt_kprintf("make elm fs on flash success, mount again\n");
            if (dfs_mount(FS_ROOT, "/", FS_NAME, 0, 0) == 0)
                rt_kprintf("mount fs on flash success\n");
            else
                rt_kprintf("mount to fs on flash fail\n");
        }
        else rt_kprintf("dfs_mkfs elm flash fail\n");
    }

    return RT_EOK;
}

INIT_ENV_EXPORT(mnt_init);