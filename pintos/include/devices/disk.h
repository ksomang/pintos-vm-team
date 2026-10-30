#ifndef DEVICES_DISK_H
#define DEVICES_DISK_H

#include <inttypes.h>
#include <stdint.h>

/* 디스크 섹터 하나의 크기(바이트). */
#define DISK_SECTOR_SIZE 512

/* 디스크 안에서 디스크 섹터의 인덱스.
 * 2 TB까지의 디스크라면 충분하다. */
typedef uint32_t disk_sector_t;

/* printf()용 형식 지정자. 예:
 * printf ("sector=%"PRDSNu"\n", sector); */
#define PRDSNu PRIu32

void disk_init (void);
void disk_print_stats (void);

struct disk *disk_get (int chan_no, int dev_no);
disk_sector_t disk_size (struct disk *);
void disk_read (struct disk *, disk_sector_t, void *);
void disk_write (struct disk *, disk_sector_t, const void *);

void 	register_disk_inspect_intr ();
#endif /* devices/disk.h */
