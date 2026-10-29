#ifndef __LIB_KERNEL_HASH_H
#define __LIB_KERNEL_HASH_H

/* Hash table.
 *
 * This data structure is thoroughly documented in the Tour of
 * Pintos for Project 3.
 *
 * This is a standard hash table with chaining.  To locate an
 * element in the table, we compute a hash function over the
 * element's data and use that as an index into an array of
 * doubly linked lists, then linearly search the list.
 *
 * The chain lists do not use dynamic allocation.  Instead, each
 * structure that can potentially be in a hash must embed a
 * struct hash_elem member.  All of the hash functions operate on
 * these `struct hash_elem's.  The hash_entry macro allows
 * conversion from a struct hash_elem back to a structure object
 * that contains it.  This is the same technique used in the
 * linked list implementation.  Refer to lib/kernel/list.h for a
 * detailed explanation. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h"

/* 안에 list_elem 하나만 있으므로, 즉 bucket 리스트에 걸리는 elem이다. */
struct hash_elem {
	struct list_elem list_elem;
};

/* hash_elem를 품고 있는 구조체의 시작 주소를 쿠하는 매크로.
   두 번째 인자(STRUCT)에 무엇을 쓰느냐에 따라 결과 타입이 정해진다.
   
   ex: struct page에 hash_elem을 넣고 hash_entry (e, struct page, 멤버 이름) 이라고 하면
   해당 struct page의 시작 주소가 나온다.
   
   멤버 이름이란 struct list_elem elem이라고 할때 타입이 struct list_elem, 멤버 이름이 elem이다.  */
#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
	((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
		- offsetof (STRUCT, MEMBER.list_elem)))

/* 원소 하나(e)를 받아서 그 원소로 계산한 숫자 하나(uin64_t, hash 값)를 돌려준다.
   hash 함수는 넣을 때 한 번, 찾을 때 한 번 불리는데 두 번 다 같은 숫자가 나와야 같은 칸에서
   넣고 찾을 수 있다. */
typedef uint64_t hash_hash_func (const struct hash_elem *e, void *aux);

/* 리스트 하나를 돌면서 찾는 원소 e와 리스트의 원소 hi를 비교한다. */
typedef bool hash_less_func (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

/* 보조 데이터 AUX를 받아 hash 원소 E에 어떤 작업을 한다. */
typedef void hash_action_func (struct hash_elem *e, void *aux);

/* Hash table.
   hash (e)로 list 칸 번호를 정하고 그 칸 리스트만 처음부터 탐색해서 less로 같은지 비교한다.
   비교하는 두 대상은 찾으려고 넘긴 원소 e와 그 칸 리스트에 있는 원소 hi이다. */
struct hash {
	size_t elem_cnt;            /* hash table에 들어 있는 원소 개수. */
	size_t bucket_cnt;          /* 칸(bucket, 리스트)의 개수 */
	struct list *buckets;       /* 칸들이 줄지어 있는 배열의 시작 주소.
								   malloc (sizeof *h->buckets * h->bucket_cnt)로 
								   struct list를 bucket_cnt개 받는다. */
	hash_hash_func *hash;       /* 원소를 몇 번 list에 넣을지 정하는 함수. */
	hash_less_func *less;       /* 같은 list 안에서 원소를 비교하는 함수. */
	void *aux;                  /* 두 함수에 넘길 추가 정보. */
};

/* A hash table iterator. */
struct hash_iterator {
	struct hash *hash;          /* The hash table. */
	struct list *bucket;        /* Current bucket. */
	struct hash_elem *elem;     /* Current hash element in current bucket. */
};

/* Basic life cycle. */
bool hash_init (struct hash *, hash_hash_func *, hash_less_func *, void *aux);
void hash_clear (struct hash *, hash_action_func *);
void hash_destroy (struct hash *, hash_action_func *);

/* Search, insertion, deletion. */
struct hash_elem *hash_insert (struct hash *, struct hash_elem *);
struct hash_elem *hash_replace (struct hash *, struct hash_elem *);
struct hash_elem *hash_find (struct hash *, struct hash_elem *);
struct hash_elem *hash_delete (struct hash *, struct hash_elem *);

/* Iteration. */
void hash_apply (struct hash *, hash_action_func *);
void hash_first (struct hash_iterator *, struct hash *);
struct hash_elem *hash_next (struct hash_iterator *);
struct hash_elem *hash_cur (struct hash_iterator *);

/* Information. */
size_t hash_size (struct hash *);
bool hash_empty (struct hash *);

/* Sample hash functions. */
uint64_t hash_bytes (const void *, size_t);
uint64_t hash_string (const char *);
uint64_t hash_int (int);

#endif /* lib/kernel/hash.h */
