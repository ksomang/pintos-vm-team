/* vm.c: 가상 메모리 객체를 위한 범용 인터페이스. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <string.h>
/* 유저 스택이 커질 수 있는 최대 크기. 과제 기준 1MB. */
#define STACK_MAX (1 << 20)
/* x86-64의 PUSH가 rsp를 줄이기 전에 rsp 아래에서 건드리는 크기. */
#define PUSH_ACCESS_SIZE 8

/* 각 하위 시스템의 초기화 코드를 호출해서
 * 가상 메모리 하위 시스템을 초기화한다. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* project 4용 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* 위의 줄들은 수정하지 말 것. */
	/* TODO: 여기에 코드를 작성하라. */
}

/* 페이지의 타입을 얻는다. 페이지가 초기화된 뒤에 어떤 타입이 될지
 * 알고 싶을 때 유용하다.
 * 이 함수는 이미 완전히 구현되어 있다. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* 보조 함수들 */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);
typedef bool page_type_initializer (struct page *, enum vm_type, void *kva);

/*지원하지 않는 type이면 NULL 반환*/
static page_type_initializer *
select_initializer (enum vm_type type){
	switch(VM_TYPE(type)){
		case VM_ANON:
			return anon_initializer;
		case VM_FILE:
			return file_backed_initializer;
		/*ANON,FILE 제외하고 나머지 타입일 경우*/
		default:
			return NULL;
	}
}

/*malloc이 실패하면 NULL을 반환함, 반환된 page는 아직 spt에 들어가지 않음*/
static struct page *
create_uninit_page (void *upage, bool writable, enum vm_type type, 
					vm_initializer *init, void *aux, page_type_initializer *initializer){
	struct page *page= malloc(sizeof (struct page));
	if(page == NULL){
		return NULL;
	}	
	uninit_new(page, pg_round_down(upage), init, type, aux, initializer);
	/* uninit_new가 *page 전체를 덮어쓰므로 writable은 반드시 uninit_new 호출 뒤에 설정한다. */
	page->writable = writable;
	return page;
}


/* initializer와 함께 대기 중인(pending) 페이지 객체를 만든다. 페이지를
 * 만들고 싶다면 직접 만들지 말고, 이 함수나 `vm_alloc_page`를 통해
 * 만들어라. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	if(spt_find_page(spt, upage) != NULL){
		return false;
	}
	page_type_initializer *selected_initializer = select_initializer(type);
	if(selected_initializer == NULL){
		return false;
	}
	struct page *page= create_uninit_page(upage, writable, type, init, aux, selected_initializer);
	if(page == NULL){
		return false;
	}
	bool insert_succ = spt_insert_page(spt,page);
	if(insert_succ == false){
		free(page);
		return false;
	}
	return true;
}


/* spt에서 VA를 찾아 페이지를 반환한다. 에러면 NULL을 반환한다. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;
	struct page tmp_page; 
	tmp_page.va = pg_round_down(va);
	struct hash_elem *found_elemental = hash_find(&spt->pages, &tmp_page.hash_elem);
	if(!found_elemental){
		return NULL;
	}
	page = hash_entry(found_elemental, struct page, hash_elem);
	return page;
}

/* 검증을 거쳐 PAGE를 spt에 삽입한다. */
bool
spt_insert_page (struct supplemental_page_table *spt ,
		struct page *page) {
	int succ = false;
	struct hash_elem *hash_insert_result = hash_insert(&spt->pages, &page->hash_elem);
	if(hash_insert_result == NULL){
		return succ = true;
	}
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* 쫓아낼(evict) struct frame을 얻는다. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: 쫓아낼 프레임을 고르는 정책은 자유롭게 정하라. */

	return victim;
}

/* 페이지 하나를 쫓아내고 그에 해당하는 프레임을 반환한다.
 * 에러면 NULL을 반환한다.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: victim을 swap out하고, 비워진 프레임을 반환하라. */

	return NULL;
}

/* palloc()으로 프레임을 얻는다. 쓸 수 있는 페이지가 없으면 페이지를 쫓아내고
 * 그 프레임을 반환한다. 이 함수는 항상 유효한 주소를 반환한다. 즉, user pool
 * 메모리가 가득 차 있으면 프레임을 쫓아내서 쓸 수 있는 메모리 공간을
 * 확보한다.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	void *physical_page = palloc_get_page(PAL_USER);
	if(physical_page == NULL){
		PANIC("todo_null_palloc");
	}
	frame = malloc(sizeof (struct frame));
	if(frame == NULL){
		PANIC("todo_null_malloc");
	}
	frame->kva = physical_page;
	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* ADDR이 RSP 기준으로 스택 접근처럼 보이면 true를 반환한다. 세 조건을 모두 만족해야 한다.
 * - RSP - PUSH_ACCESS_SIZE 이상: 정상적인 스택 접근은 rsp 위에서 일어난다.
 *   단 PUSH는 rsp를 줄이기 전에 그 아래를 먼저 건드리므로 그만큼은 허용한다.
 *   그보다 더 아래(예: rsp - 4096)는 잘못된 접근으로 본다. (pt-grow-bad)
 * - USER_STACK 미만: 유저 스택 영역 안이어야 한다.
 * - USER_STACK - STACK_MAX 이상: 스택은 최대 1MB까지만 커질 수 있다.
 * vm_try_handle_fault와 check_address가 같은 기준을 쓰도록 이 함수 하나를 공유한다. */
bool
is_stack_access(void *addr, uintptr_t rsp){
	if((uintptr_t) addr >= rsp - PUSH_ACCESS_SIZE && (uintptr_t) addr < USER_STACK && (uintptr_t) addr >= USER_STACK - STACK_MAX){
		return true;
	}
	return false;
}

/* 스택을 키운다. ADDR이 들어 있는 페이지를 스택 페이지(VM_ANON | VM_STACK)로 SPT에 등록만 한다.
 * 스택 접근인지는 호출하는 쪽(vm_try_handle_fault)이 미리 판단하고,
 * claim도 그쪽의 원래 흐름(vm_do_claim_page)이 한다. claim 때 anon_initializer가 0으로 채운다.
 * SPT는 페이지 경계 주소로 관리하므로 ADDR을 내려서 등록한다.
 * 등록에 실패하면 false를 반환한다. */
static bool
vm_stack_growth (void *addr) {
	addr = pg_round_down(addr);
	return vm_alloc_page (VM_ANON | VM_STACK, addr, true);
}

/* 쓰기 보호된(write_protected) 페이지에서 난 fault를 처리한다 */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* page fault를 처리한다. 성공하면 true를 반환한다.
 * - SPT에 있는 페이지면 프레임을 붙여 내용을 채운다. (lazy loading)
 * - SPT에 없지만 스택 접근이면 스택 페이지를 새로 만들어 같은 방식으로 올린다. (stack growth)
 * 커널 주소, 이미 올라온 페이지의 권한 위반, 스택이 아닌 미등록 주소,
 * 읽기 전용 페이지에 쓰기는 처리할 수 없으므로 false를 반환한다. */
bool
vm_try_handle_fault (struct intr_frame *f, void *addr,
		bool user, bool write , bool not_present ) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;

	/*유저 영역 주소가 아니면 처리할 수 없다*/
	if(addr == NULL || is_user_vaddr(addr) == false){
		return false;
	}
	/*이미 매핑된 페이지의 권한 위반이다*/
	if(not_present == false){
		return false;
	}
	page = spt_find_page(spt, addr);
	/*SPT에 없는 주소면 스택 접근일 때만 새 스택 페이지를 만들고, 아니면 처리할 수 없다.
	 *유저 모드 fault는 f->rsp가 유저 rsp지만, 커널 모드 fault(syscall 중 유저 버퍼 접근)는
	 *f->rsp가 커널 스택이라 syscall_handler가 저장해 둔 user_rsp를 쓴다.
	 *만든 페이지를 다시 찾아 아래의 원래 흐름(writable 검사 -> claim)으로 이어간다.*/
	if(page == NULL){
		uintptr_t rsp = user ? f->rsp : thread_current ()->user_rsp;
		if(!is_stack_access (addr, rsp)){
			return false;
		}
		if(!vm_stack_growth (addr)){
			return false;
		}
		page = spt_find_page (spt, addr);
	}
	/*읽기 전용 페이지에 쓰려는 접근*/
	if(write == true && page->writable ==false){
		return false;
	}
	return vm_do_claim_page (page);
}

/* 페이지를 해제한다.
 * 이 함수는 수정하지 말 것. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* VA에 할당된 페이지를 claim한다. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;
	page = spt_find_page(&thread_current()->spt, va);
	if(page == NULL){
		return false;
	}
	return vm_do_claim_page (page);
}

/* PAGE를 claim하고 mmu를 설정한다. */
/* TODO: swap_in 실패 시 정리 필요.
 * pml4_clear_page -> palloc_free_page(frame->kva) -> free(frame) -> page->frame = NULL 순서로 메모리 해제. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* 연결 설정 */
	frame->page = page;
	page->frame = frame;

	bool result = pml4_set_page(thread_current()->pml4, page->va,frame->kva, page->writable);
	if(result==false){
		palloc_free_page(frame->kva);
		free(frame);
		page->frame = NULL;
		return false;
	}
	return swap_in (page, frame->kva);
}

static uint64_t
page_hash (const struct hash_elem *e, void *aux UNUSED) {
	const struct page *p = hash_entry(e, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

static bool
page_less (const struct hash_elem *a,
		   const struct hash_elem *b, void *aux UNUSED) {
	const struct page *pa = hash_entry (a, struct page, hash_elem);
	const struct page *pb = hash_entry (b, struct page, hash_elem);
	return pa->va < pb->va;
}

/* 새 보조 페이지 테이블(supplemental page table)을 초기화한다 */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	hash_init(&spt->pages, page_hash, page_less, NULL);
}

/* 보조 페이지 테이블을 src에서 dst로 복사한다.
 * fork 때 자식 스레드의 __do_fork에서 불린다. dst는 자식(현재 스레드)의 SPT, src는 부모의 SPT다.
 * 부모는 fork_sema에서 기다리고 있으므로 복사하는 동안 src는 바뀌지 않는다.
 * 실패하면 false를 돌려준다. 반쯤 만든 dst는 __do_fork의 error 경로에서
 * process_cleanup -> supplemental_page_table_kill이 정리하므로 여기서 따로 지우지 않는다. */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src) {
	/* vm_alloc_page, vm_claim_page는 현재 스레드의 SPT에 작용하므로 dst가 현재 스레드 것이어야 한다. */
	ASSERT (dst == &thread_current ()->spt);

	struct hash_iterator i;
	hash_first(&i, &src->pages);
	while(hash_next(&i)){
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		void *va = src_page->va;
		bool writable = src_page->writable;

		/* 지금 상태로 나눈다. page_get_type은 uninit이어도 나중 타입(VM_ANON 등)을 돌려주므로
		 * 아직 안 올라온 페이지인지는 operations->type으로 판별한다. */
		if(VM_TYPE(src_page->operations->type) == VM_UNINIT){
			/* 1. 아직 안 올라온 페이지: 부모와 같은 lazy loading 정보로 자식 페이지를 등록한다.
			 * aux는 페이지마다 따로 해제되므로(lazy_load_segment, uninit_destroy)
			 * 부모 것을 그대로 넘기면 이중 해제가 난다. 그래서 자식용 복사본을 만든다.
			 * aux가 NULL인 페이지(init 없이 등록된 anon 페이지)는 복사할 것이 없으니 NULL 그대로 넘긴다. */
			struct aux_info *aux_copy = NULL;
			if(src_page->uninit.aux != NULL){
				aux_copy = malloc(sizeof(struct aux_info));
				if(aux_copy == NULL){
					return false;
				}
				memcpy(aux_copy, src_page->uninit.aux, sizeof (struct aux_info));
				aux_copy->file_ptr = thread_current ()->running_file;
			}
			/* operations->type은 VM_UNINIT이라 ASSERT에 걸리므로 원래 타입(마커 포함)인 uninit.type을 넘긴다.
			 * 등록에 실패하면 aux_copy를 가진 페이지가 없으니 여기서 해제한다.
			 * 등록에 성공하면 claim하지 않고 uninit 그대로 둔다. 자식이 실제로 접근할 때 lazy load된다. */
			if(!vm_alloc_page_with_initializer(src_page->uninit.type, src_page->va, src_page->writable, src_page->uninit.init, aux_copy)){
				free(aux_copy);
				return false;
			}
		}
		else{
			/* 2. 이미 올라온 페이지: 부모가 실행하면서 바꾼 내용이 있으므로 파일에서 다시 읽지 않고
			 * 새 프레임을 받아 부모 프레임 내용을 그대로 복사한다. */
			if(!vm_alloc_page(page_get_type (src_page), va, writable)){
				return false;
			}
			if(!vm_claim_page(va)){
				return false;
			}
			/* claim 중에 anon_initializer가 프레임을 0으로 채우므로 복사는 반드시 claim 다음에 한다.
			 * kva는 커널 주소라서 지금 켜진 pml4(자식)와 상관없이 부모 프레임을 읽을 수 있다. */
			struct page *dst_page = spt_find_page (dst, va);
			memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
		}
	}
	return true;
}

static void
destructor(struct hash_elem *e, void *aux UNUSED){
	struct page *destroy_elem = hash_entry(e, struct page, hash_elem);
	vm_dealloc_page(destroy_elem);
}

/* 보조 페이지 테이블이 가진 자원을 해제한다 */
void
supplemental_page_table_kill (struct supplemental_page_table *spt) {
	hash_clear(&spt->pages, destructor);
}