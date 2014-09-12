#ifndef _LINUX_GLAMOFB_H
#define _LINUX_GLAMOFB_H

#include <linux/fb.h>


#define GLAMOFB_ENGINE_ENABLE _IOW('F', 0x1, __u32)
#define GLAMOFB_ENGINE_DISABLE _IOW('F', 0x2, __u32)
#define GLAMOFB_ENGINE_RESET _IOW('F', 0x3, __u32)

#endif
