/* anon.c: 디스크 이미지가 없는 페이지(즉 anonymous page)의 구현. */

#include "vm/vm.h"
#include "devices/disk.h"

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

/* 파일 매핑을 초기화한다 */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 핸들러를 설정한다 */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
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
}
