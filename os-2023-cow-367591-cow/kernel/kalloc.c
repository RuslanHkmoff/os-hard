// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "constants.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "types.h"

void freerange(void *pa_start, void *pa_end);

extern char end[];  // first address after kernel.
                    // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  // int ref_count[CONVERT(PHYSTOP)];
} kmem;

struct {
  int ref_array[CONVERT(PHYSTOP)];
  struct spinlock lock;

} ref_count;
void kinit() {
  initlock(&kmem.lock, "kmem");
  initlock(&ref_count.lock, "ref_count");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    kfree(p);
  }
}

void decrement_ref_count(uint64 pa) {
  acquire(&ref_count.lock);
  ref_count.ref_array[pa]--;
  release(&ref_count.lock);
}

int is_single_owner(uint64 pa) {
  acquire(&ref_count.lock);
  int value = ref_count.ref_array[pa];
  release(&ref_count.lock);
  if (value == 1) return 1;
  return 0;
}

void increment_ref_count(uint64 pa) {
  acquire(&ref_count.lock);
  ref_count.ref_array[pa]++;
  release(&ref_count.lock);
}

int get_ref_count(uint64 pa) {
  acquire(&ref_count.lock);
  int value = ref_count.ref_array[pa];
  release(&ref_count.lock);
  return value;
}

void set_ref_count(uint64 pa, int update) {
  acquire(&ref_count.lock);
  ref_count.ref_array[pa] = update;
  release(&ref_count.lock);
}

int decrement_and_get(uint64 pa) {
  acquire(&ref_count.lock);
  ref_count.ref_array[pa]--;
  int res = ref_count.ref_array[pa];
  release(&ref_count.lock);
  return res;
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if (decrement_and_get((CONVERT((uint64)pa))) > 0) {
    return;
  }
  // if (--ref_count.ref_array[CONVERT((uint64)pa)] > 0) {
  //   return;
  // }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) kmem.freelist = r->next;
  release(&kmem.lock);

  if (r) memset((char *)r, 5, PGSIZE);  // fill with junk
  if (r) {
    set_ref_count(CONVERT((uint64)r), 1);
    // ref_count.ref_array[CONVERT((uint64)r)] = 1;
  }
  return (void *)r;
}
