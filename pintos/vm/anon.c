/* anon.c: 디스크 이미지가 없는 페이지(즉 anonymous page)의 구현. */

#include <string.h>
#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h"

/* 아래 줄은 수정하지 말 것 */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* 이 구조체는 수정하지 말 것 */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* anonymous page를 위한 데이터를 초기화한다 */
void
vm_anon_init (void) {
	/* TODO: swap_disk를 설정하라. */
	swap_disk = NULL;
}

/* uninit 페이지를 anonymous page로 바꾼다.
 * 페이지가 처음 claim될 때 uninit_initialize가 한 번 부른다. */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 핸들러를 설정한다 */
	page->operations = &anon_ops;

	/* anonymous page는 0으로 시작해야 한다.
	 * vm_get_frame이 준 프레임에는 이전에 쓰던 프로세스의 데이터(또는 0xcc)가 남아 있을 수 있어서,
	 * 그대로 넘기면 다른 프로세스의 데이터가 보이고 실행할 때마다 결과가 달라질 수 있다.
	 * 프레임을 받을 때(vm_get_frame)가 아니라 여기서 채우는 이유:
	 * - anon page가 처음 만들어질 때 딱 한 번만 실행된다. swap in은 이 함수를 거치지 않으므로 복원할 내용을 덮지 않는다.
	 * - 스택, stack growth 페이지 등 init 없이 만드는 모든 anon page에 똑같이 적용된다.
	 * - eviction으로 재사용된 프레임(palloc을 거치지 않음)도 빠짐없이 0이 된다.
	 * 실행 파일 세그먼트는 init(lazy_load_segment)이 이 다음에 파일 내용으로 다시 채운다. */
	memset (kva, 0, PGSIZE);

	struct anon_page *anon_page = &page->anon;
	return true;
}

/* swap disk에서 내용을 읽어 페이지를 swap in한다. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* 내용을 swap disk에 써서 페이지를 swap out한다. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* anonymous page를 파괴한다. PAGE 자체는 호출한 쪽에서 해제한다. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	free(page->frame);
}
