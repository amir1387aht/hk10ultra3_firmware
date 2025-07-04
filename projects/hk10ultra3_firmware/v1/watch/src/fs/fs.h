#ifndef FS_UTILS_H
#define FS_UTILS_H

#include "dfs_file.h"
#include "dfs_fs.h"
#include "dfs_posix.h"
#include "drv_flash.h"
#include <custom_mem_map.h>
#include <rtthread.h>

int mnt_init(void);

#endif // FS_UTILS_H