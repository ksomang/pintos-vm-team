/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/process.h"
#include <string.h>

/* uninit page의 page_initializer와 같은 모양의 함수 타입. */
typedef bool type_initializer_func (struct page *, enum vm_type, void *);

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
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

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* type의 종류(VM_TYPE)에 맞는 page initializer를 돌려준다.
   지원하지 않는 종류면 NULL. */
static 
type_initializer_func *initializer_for_type (enum vm_type type) {

	if (VM_TYPE(type) == VM_ANON) {
		return anon_initializer; 
	}
	else if (VM_TYPE(type) == VM_FILE) {
		return file_backed_initializer;
	}

	return NULL;
}

/* upage에 uninit page 하나를 만들어 현재 스레드의 SPT에 넣는다.
   첫 page fault 때 type에 맞는 initializer와 init(aux)가 불려 내용이 채워진다.
   frame 할당, 내용 채우기, pml4 매핑은 여기서 하지 않는다.
   성공하면 true, upage에 이미 page가 있거나 메모리가 부족하거나 지원하지 않는 type이거나 SPT 삽입에 실패하면 false를 반환한다. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	if (spt_find_page (spt, upage) == NULL) {
		struct page *page = malloc (sizeof (struct page));

		if (page == NULL) {
			goto err;
		}

		type_initializer_func *type_initializer = initializer_for_type (type);

		if (type_initializer == NULL) {
			free (page);
			goto err;
		}

		uninit_new (page, upage, init, type, aux, type_initializer);

		/* uninit_new는 writable을 0으로 초기화하므로 uninit_new가 호출되기 전에 대입하지 말아야 한다. */
		page->writable = writable;

		if (!spt_insert_page (spt, page)) {
			free (page);
			goto err;
		}

		return true;
	}
err:
	return false;
}

/* hash_find는 va가 아니라 hash_elem을 받는다.
   page_va_hash와 page_va_less는 va만 읽으므로,
   va만 채운 임시 struct page의 elem을 넘겨 검색한다. 
   이 page는 hash에 넣지 않는다. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;
	/* spt_find_page가 가진 것은 spt와 va뿐이다. struct page는 가지고 있지 않다.
	   struct page 변수를 선언해 va와 elem이 들어갈 자리를 만든다. */
	struct page tmp_page;

	tmp_page.va = pg_round_down(va);
	struct hash_elem *found_elem = hash_find (&spt->page_hash, &tmp_page.elem);

	if (found_elem == NULL) {
		return NULL;
	}

	page = hash_entry (found_elem, struct page, elem);

	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt,
		struct page *page) {
	bool succ = false;
	
	struct hash_elem *existing_elem = hash_insert (&spt->page_hash, &page->elem);

	if (existing_elem == NULL) {
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* user pool에서 물리 page를 하나 받고 그걸 담을 struct frame을 만들어 돌려준다.
   frame의 멤버들도 초기화한다. */
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	void *kpage = palloc_get_page (PAL_USER);

	if (kpage == NULL) {
		PANIC ("todo: no free frame in user pool");
	}

	frame = malloc (sizeof (struct frame));

	if (frame == NULL) {
		palloc_free_page (kpage);
		PANIC ("malloc failed for struct frame");
	}
	
	frame->kva = kpage;
	frame->page = NULL;

	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* page fault를 처리한다. 지금은 lazy loading으로 아직 안 올라온 page만 고친다.
   읽기 전용 page에 쓰려 했거나(not_present가 false) SPT에 없는 주소면 false를 반환하고
   SPT에 있으면 frame을 붙여 내용을 채운 뒤 그 결과를 반환한다. */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr,
		bool user UNUSED, bool write UNUSED, bool not_present) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;

	if (!not_present) {
		return false;
	}

	page = spt_find_page (spt, addr);

	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;
	
	page = spt_find_page (&thread_current ()->spt, va);

	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	if (!pml4_set_page (thread_current ()->pml4, page->va, frame->kva, page->writable)) {
		palloc_free_page (frame->kva);
		free (frame);
		page->frame = NULL;

		return false;
	}

	return swap_in (page, frame->kva);
}

static uint64_t 
page_va_hash (const struct hash_elem *e, void *aux UNUSED) {
	struct page *page = hash_entry (e, struct page, elem);
	uint64_t hash = hash_bytes (&page->va, 8);
	
	return hash;
}

static bool 
page_va_less (const struct hash_elem *a, const struct hash_elem *b,
		void *aux UNUSED) {
	struct page *a_page = hash_entry (a, struct page, elem);
	struct page *b_page = hash_entry (b, struct page, elem);
	
	if (a_page->va < b_page->va) {
		return true;
	}
	else {
		return false;
	}
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	hash_init (&spt->page_hash, &page_va_hash, &page_va_less, NULL);
}

/* 부모의 uninit page를 자식 SPT에 복사한다.
   내용이 없으니 claim하지 않고 같은 init과 aux 내용을 가진 uninit page로 등록만 한다.
   aux는 lazy_load_segment가 쓰고 나서 free하므로 부모와 같은 aux를 가리키면 두 번 free된다.
   그래서 aux는 새로 malloc해 내용을 베낀 뒤 넘긴다.
   메모리 할당이나 등록에 실패하면 false를 반환한다. */
static bool
duplicate_uninit_page (struct page *src_page) {
	struct segment_aux_info *dst_aux = malloc (sizeof (struct segment_aux_info));
	
	if (dst_aux == NULL) {
		return false;
	}
	
	memcpy (dst_aux, src_page->uninit.aux, sizeof (struct segment_aux_info));
	
	if (!vm_alloc_page_with_initializer (page_get_type (src_page), src_page->va, src_page->writable, src_page->uninit.init, dst_aux)) {
		free (dst_aux);
		return false;
	}

	return true;
}

/* 부모의 anon page를 자식 SPT에 복사한다.
   같은 주소에 page를 등록하고 바로 claim해 자식만의 새 frame을 붙인 뒤
   부모 frame의 내용을 그대로 베낀다.
   등록이나 claim에 실패하면 false를 반환한다. */
static bool
duplicate_anon_page (struct supplemental_page_table *dst, struct page *src_page) {
	if (!vm_alloc_page_with_initializer (page_get_type (src_page), src_page->va, src_page->writable, NULL, NULL)) {
		return false;
	}
	
	if (!vm_claim_page (src_page->va)) {
		return false;
	}
	
	struct page *dst_page = spt_find_page (dst, src_page->va);
	memcpy (dst_page->frame->kva, src_page->frame->kva, PGSIZE);

	return true;
}

/* fork 때 부모의 SPT를 자식의 SPT로 복사한다.
   자식 스레드가 부르므로 page 등록과 claim은 자식의 SPT와 page table에 들어간다.
   page의 지금 상태(operations->type)에 따라 uninit이면 init과 aux만 복제해 uninit page로 등록하고 그 밖이면 내용까지 복사한다.
   하나라도 실패하면 false를 반환한다. */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src) {
	struct hash_iterator i;

	hash_first (&i, &src->page_hash);

	while (hash_next (&i))
	{
		struct page *src_page = hash_entry (hash_cur (&i), struct page, elem);

		if (src_page->operations->type == VM_UNINIT) {
			if (!duplicate_uninit_page (src_page)) {
				return false;
			}
		}
		else {
			if (!duplicate_anon_page (dst, src_page)) {
				return false;
			}
		}
	}

	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
