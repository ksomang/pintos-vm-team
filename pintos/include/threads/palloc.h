#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stdint.h>
#include <stddef.h>

/* 페이지를 할당하는 방식. */
enum palloc_flags {
	PAL_ASSERT = 001,           /* 실패하면 panic. */
	PAL_ZERO = 002,             /* 페이지 내용을 0으로 채움. */
	PAL_USER = 004              /* 유저 페이지. */
};

/* user pool에 넣을 최대 페이지 수. */
extern size_t user_page_limit;

uint64_t palloc_init (void);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

#endif /* threads/palloc.h */
