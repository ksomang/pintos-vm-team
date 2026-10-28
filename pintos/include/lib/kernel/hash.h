#ifndef __LIB_KERNEL_HASH_H
#define __LIB_KERNEL_HASH_H

/* 해시 테이블.
 *
 * 이 자료구조는 Project 3의 "Tour of Pintos"에 자세히 설명되어
 * 있다.
 *
 * 이것은 체이닝(chaining)을 사용하는 표준 해시 테이블이다.
 * 테이블에서 원소를 찾으려면, 원소의 데이터에 해시 함수를 적용해
 * 값을 계산하고, 그 값을 이중 연결 리스트 배열의 인덱스로 사용한
 * 뒤, 해당 리스트를 선형 탐색한다.
 *
 * 체인 리스트는 동적 할당을 사용하지 않는다. 대신 해시에 들어갈
 * 수 있는 모든 구조체는 struct hash_elem 멤버를 내장(embed)해야
 * 한다. 모든 해시 함수는 이 `struct hash_elem'을 대상으로
 * 동작한다. hash_entry 매크로를 사용하면 struct hash_elem에서
 * 그것을 포함하는 구조체 객체로 다시 변환할 수 있다. 이것은 연결
 * 리스트 구현에서 사용한 것과 같은 기법이다. 자세한 설명은
 * lib/kernel/list.h를 참고하라. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h"

/* 해시 원소. */
struct hash_elem {
	struct list_elem list_elem;
};

/* 해시 원소 HASH_ELEM에 대한 포인터를, HASH_ELEM이 내장되어 있는
 * 구조체에 대한 포인터로 변환한다. 바깥 구조체의 이름 STRUCT와,
 * 해시 원소의 멤버 이름 MEMBER를 넘겨라. 예시는 파일 맨 위의 큰
 * 주석을 참고하라. */
#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
	((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
		- offsetof (STRUCT, MEMBER.list_elem)))

/* 보조 데이터 AUX가 주어졌을 때, 해시 원소 E의 해시값을 계산해
 * 반환한다. */
typedef uint64_t hash_hash_func (const struct hash_elem *e, void *aux);

/* 보조 데이터 AUX가 주어졌을 때, 두 해시 원소 A와 B의 값을
 * 비교한다. A가 B보다 작으면 true를, A가 B보다 크거나 같으면
 * false를 반환한다. */
typedef bool hash_less_func (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

/* 보조 데이터 AUX가 주어졌을 때, 해시 원소 E에 어떤 작업을
 * 수행한다. */
typedef void hash_action_func (struct hash_elem *e, void *aux);

/* 해시 테이블. */
struct hash {
	size_t elem_cnt;            /* 테이블에 있는 원소 개수. */
	size_t bucket_cnt;          /* 버킷 개수. 2의 거듭제곱. */
	struct list *buckets;       /* `bucket_cnt'개의 리스트로 이루어진 배열. */
	hash_hash_func *hash;       /* 해시 함수. */
	hash_less_func *less;       /* 비교 함수. */
	void *aux;                  /* `hash'와 `less'에 넘길 보조 데이터. */
};

/* 해시 테이블 반복자(iterator). */
struct hash_iterator {
	struct hash *hash;          /* 해시 테이블. */
	struct list *bucket;        /* 현재 버킷. */
	struct hash_elem *elem;     /* 현재 버킷 안의 현재 해시 원소. */
};

/* 기본 생명주기. */
bool hash_init (struct hash *, hash_hash_func *, hash_less_func *, void *aux);  /* 해시 테이블을 쓸 수 있게 준비함 */
void hash_clear (struct hash *, hash_action_func *);                            /* 원소만 전부 비우고, 해시 테이블은 계속 쓸 수 있게 남겨 둔다 */
void hash_destroy (struct hash *, hash_action_func *);                          /* hash_clear를 한 다음 버킷 배열까지 free한다 -> 아예 부숨, hash_init 전까지 재사용 금지 */

/* 검색, 삽입, 삭제. */
/*find_bucket: hash(e)의 값으로 버킷을 고름
 *find_elem: 그 버킷 리스트를 돌면서 !less(a,b) && !less(b,a)인 원소를 찾음. */
struct hash_elem *hash_insert (struct hash *, struct hash_elem *);              /* 같은 원소가 없으면 new 넣고 NULL 반환, 같은 원소 있으면 new 삽입 안하고 그 같은 원소 반환 */
struct hash_elem *hash_replace (struct hash *, struct hash_elem *);             /* 같은 원소가 있으면 그 같은 원소 빼고 new로 바꿔 넣음, 뺐던 기존 원소 반환, 빠진 원소 메모리 해제는 호출한 쪽이 해야됨 */
struct hash_elem *hash_find (struct hash *, struct hash_elem *);                /* 두번째 인자(e)와 같다고 판단되는 원소를 찾아서 반환, 없으면 NULL 반환 */
struct hash_elem *hash_delete (struct hash *, struct hash_elem *);              /* 두번째 인자(e)와 같은 원소 찾아서 해시에서 빼고 반환, 없으면 NULL 반환 */

/* 순회. */
void hash_apply (struct hash *, hash_action_func *);                            /* 모든 원소에 대해 action을 한 번씩 호출, 순서는 랜덤 */
void hash_first (struct hash_iterator *, struct hash *);                        /* 첫 번째 버킷의 head(맨 앞의 더미)를 가리키게 함 */
struct hash_elem *hash_next (struct hash_iterator *);                           /* 다음 원소로 이동, 버킷이 끝나면 다음 버킷으로 넘어가고, 전부 끝나면 NULL 반환 */
struct hash_elem *hash_cur (struct hash_iterator *);                            /* 지금 가리키는 원소를 반환 */

/* 정보 조회. */
size_t hash_size (struct hash *);                                               /* 원소 개수 (elem_cnt) */
bool hash_empty (struct hash *);                                                /* 원소가 0개면 true */

/* 예시 해시 함수들. */
uint64_t hash_bytes (const void *, size_t);                                     /* BUF에 있는 SIZE 바이트의 해시값을 반환한다. */
uint64_t hash_string (const char *);                                            /* 문자열 S의 해시값을 반환한다. */
uint64_t hash_int (int);                                                        /* 정수 I의 해시값을 반환한다. */

#endif /* lib/kernel/hash.h */
