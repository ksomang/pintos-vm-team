/* vm.c: 가상 메모리 객체를 위한 범용 인터페이스. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
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
typedef bool page_initializer (struct page *, enum vm_type, void *kva);

/*initializer 선택하기*/
static page_initializer *
select_initializer (enum vm_type type){
	switch(VM_TYPE(type)){
		case VM_ANON:
			return anon_initializer;
		case VM_FILE:
			return file_backed_initializer;
		/*ANON/FILE만 지원한다 (VM_PAGE_CACHE는 project 4)*/
		default:
			return NULL;
	}
}

/*uninit_new하기*/
static struct page *
create_uninit_page (void *upage, bool writable, enum vm_type type, 
					vm_initializer *init, void *aux, page_initializer *initializer){
	struct page *page= malloc(sizeof (struct page));
	if(page == NULL){
		return NULL;
	}	
	uninit_new(page, pg_round_down(upage), init, type, aux, initializer);
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
	page_initializer *selected_initializer = select_initializer(type);
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

/* 스택을 키운다. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* 쓰기 보호된(write_protected) 페이지에서 난 fault를 처리한다 */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* 성공하면 true를 반환한다 */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: fault가 유효한지 검사하라 */
	/* TODO: 여기에 코드를 작성하라 */

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

/* 보조 페이지 테이블을 src에서 dst로 복사한다 */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* 보조 페이지 테이블이 가진 자원을 해제한다 */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: 스레드가 가진 supplemental_page_table을 전부 파괴하고,
	 * TODO: 수정된 내용은 모두 저장소에 다시 써라(writeback). */
}