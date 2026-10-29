/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

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

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
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
		PANIC ("TODO");
	}

	frame = malloc (sizeof (struct frame));

	if (frame == NULL) {
		palloc_free_page (kpage);
		PANIC ("PANIC");
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

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

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
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: rw는 vm_alloc_page_with_initializer 때 완성한다. */
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

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
