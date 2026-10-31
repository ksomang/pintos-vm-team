/* uninit.c: 초기화되지 않은(uninit) 페이지의 구현.
 *
 * 모든 페이지는 uninit 페이지로 태어난다. 첫 page fault가 일어나면
 * 핸들러 호출 사슬이 uninit_initialize (page->operations.swap_in)를 호출한다.
 * uninit_initialize 함수는 페이지 객체를 초기화해서 페이지를 특정 페이지
 * 객체(anon, file, page_cache)로 바꾸고(transmute),
 * vm_alloc_page_with_initializer 함수에서 넘겨받은 초기화 콜백을
 * 호출한다.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* 이 구조체는 수정하지 말 것 */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* 이 함수는 수정하지 말 것 */
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* 지금은 프레임 없음 */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* 첫 fault 때 페이지를 초기화한다 */
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* 먼저 꺼내 둔다. page_initialize가 이 값들을 덮어쓸 수 있다 */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: 이 함수를 고쳐야 할 수도 있다. */
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* uninit_page가 가진 자원을 해제한다. 대부분의 페이지는 다른 페이지 객체로
 * 바뀌지만, 실행 중에 한 번도 참조되지 않은 uninit 페이지가 프로세스 종료 시점에
 * 남아 있을 수 있다.
 * PAGE 자체는 호출한 쪽에서 해제한다. */
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: 이 함수를 채워라.
	 * TODO: 할 일이 없다면 그냥 return하라. */
}
