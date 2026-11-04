#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

/* 파일 시스템 lock. process.c에서도 같이 써야 해서 꺼내 둔다. */
extern struct lock filesys_lock;

/* filesys_lock을 안 쥐고 있을 때만 잡는다. 잡았으면 true.
   반환값은 filesys_lock_release_if_acquired에 넘겨서 푼다. */
bool filesys_lock_acquire_if_needed (void);

/* filesys_lock_acquire_if_needed가 잡았을 때(ACQUIRED가 true)만 filesys_lock을 푼다. */
void filesys_lock_release_if_acquired (bool acquired);

void syscall_init (void);

/* exit할 때 열린 파일 다 닫고 fd 표 반납 */
void destroy_fd_table (void);

/* fork할 때 부모 fd 표를 자식에게 복제 */
struct thread;
bool duplicate_fd_table (struct thread *parent);

#endif /* userprog/syscall.h */