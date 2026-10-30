#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

/* 비트맵 추상 자료형. */

/* 생성과 파괴. */
struct bitmap *bitmap_create (size_t bit_cnt);
struct bitmap *bitmap_create_in_buf (size_t bit_cnt, void *, size_t byte_cnt);
size_t bitmap_buf_size (size_t bit_cnt);
void bitmap_destroy (struct bitmap *);

/* 비트맵 크기. */
size_t bitmap_size (const struct bitmap *);

/* 비트 하나 설정하기와 검사하기. */
void bitmap_set (struct bitmap *, size_t idx, bool);
void bitmap_mark (struct bitmap *, size_t idx);
void bitmap_reset (struct bitmap *, size_t idx);
void bitmap_flip (struct bitmap *, size_t idx);
bool bitmap_test (const struct bitmap *, size_t idx);

/* 여러 비트 설정하기와 검사하기. */
void bitmap_set_all (struct bitmap *, bool);
void bitmap_set_multiple (struct bitmap *, size_t start, size_t cnt, bool);
size_t bitmap_count (const struct bitmap *, size_t start, size_t cnt, bool);
bool bitmap_contains (const struct bitmap *, size_t start, size_t cnt, bool);
bool bitmap_any (const struct bitmap *, size_t start, size_t cnt);
bool bitmap_none (const struct bitmap *, size_t start, size_t cnt);
bool bitmap_all (const struct bitmap *, size_t start, size_t cnt);

/* 설정된(1) 비트 또는 설정 안 된(0) 비트 찾기. */
#define BITMAP_ERROR SIZE_MAX
size_t bitmap_scan (const struct bitmap *, size_t start, size_t cnt, bool);
size_t bitmap_scan_and_flip (struct bitmap *, size_t start, size_t cnt, bool);

/* 파일 입출력. */
#ifdef FILESYS
struct file;
size_t bitmap_file_size (const struct bitmap *);
bool bitmap_read (struct bitmap *, struct file *);
bool bitmap_write (const struct bitmap *, struct file *);
#endif

/* 디버깅. */
void bitmap_dump (const struct bitmap *);

#endif /* lib/kernel/bitmap.h */
