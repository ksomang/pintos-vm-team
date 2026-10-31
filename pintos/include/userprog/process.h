#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "filesys/file.h"

struct segment_aux_info {
	struct file *file;
	off_t ofs;
	uint32_t read_bytes;	/* page 하나만큼의 read_bytes. */
	uint32_t zero_bytes;	/* page 하나만큼의 zero_bytes. */
};

tid_t process_create_initd (const char *file_name);   /* 첫 프로세스 */
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);                       /* 성공하면 안 돌아옴 */
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

#endif /* userprog/process.h */