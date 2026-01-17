// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  char quota[PHYSTOP / PGSIZE];
} kmapcnt;

void
kinit()
{
  initlock(&kmapcnt.lock, "kmapcnt");
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    acquire(&kmapcnt.lock);
    kmapcnt.quota[(uint64)p / PGSIZE] = 1; 
    release(&kmapcnt.lock);
    kfree(p);
  }
}

void kaddmapcnt(void *pa) {

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return ;

  acquire(&kmapcnt.lock);
  kmapcnt.quota[(uint64)pa / PGSIZE] ++; 
  release(&kmapcnt.lock);
}

int kgetmapcnt(void *pa) {
  return kmapcnt.quota[(uint64)pa / PGSIZE];
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmapcnt.lock);
  int refcnt = --kmapcnt.quota[(uint64)pa / PGSIZE];

  if(refcnt == 0) {
    acquire(&kmem.lock);
    r = (struct run*)pa;

    release(&kmapcnt.lock);
    
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    
    r->next = kmem.freelist;
    kmem.freelist = r;
    
    release(&kmem.lock);
  } else {
    release(&kmapcnt.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
  }
  release(&kmem.lock);
  
  if(r) {
    // 分配成功后设置引用计数
    acquire(&kmapcnt.lock);
    kmapcnt.quota[(uint64)r / PGSIZE] = 1;
    release(&kmapcnt.lock);
    
    memset((char*)r, 5, PGSIZE);
  }
  
  return (void*)r;
}
