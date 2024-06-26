#ifndef _MODULE_H_
#define _MODULE_H_

/* MODULE DEFINITIONS */
#define MAJOR_NUM 242
#define DEV_NAME "music_driver"
#define DEV_FILE_LOC "/dev/music_driver"
/* IOCTL NUMBERS */
#define IOCTL_OPTION _IOW(MAJOR_NUM, 20211584, char *)

#endif /* _MODULE_H_ */