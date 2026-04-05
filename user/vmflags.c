#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

volatile int global_value = 111;

static void
fail(const char *msg)
{
  printf("vmflags: %s\n", msg);
  exit(1);
}

static void
check_eq(const char *label, int got, int want)
{
  if(got != want){
    printf("vmflags: %s: got %d, want %d\n", label, got, want);
    exit(1);
  }
}

static void
show_flags(const char *label, void *addr, uint64 len)
{
  int a = vmcheckflags(addr, len, PTE_A);
  int d = vmcheckflags(addr, len, PTE_D);
  int ad = vmcheckflags(addr, len, PTE_A | PTE_D);

  printf("%s addr=%p len=%ld A=%d D=%d A|D=%d\n", label, addr, len, a, d, ad);
}

static void
dump_stage(const char *label)
{
  printf("\n=== %s ===\n", label);
  vmprint();
}

int
main(void)
{
  volatile int stack_value = 222;
  volatile uchar stack_array[PGSIZE / 2];
  volatile uchar *heap_array;
  volatile int rd;
  int heap_bytes = 3 * PGSIZE;

  stack_array[321] = 33;

  dump_stage("start");
  show_flags("global value", (void *)&global_value, sizeof(global_value));
  show_flags("stack value", (void *)&stack_value, sizeof(stack_value));
  show_flags("stack array item", (void *)&stack_array[321], sizeof(stack_array[321]));

  heap_array = (volatile uchar *)sbrk(heap_bytes);
  if((char *)heap_array == SBRK_ERROR) fail("sbrk(heap) failed");

  heap_array[0] = 1;
  heap_array[PGSIZE] = 2;
  heap_array[2 * PGSIZE + 42] = 3;

  dump_stage("after allocation");
  show_flags("heap array", (void *)heap_array, heap_bytes);

  if(vmclearflags((void *)0, (uint64)sbrk(0), PTE_A | PTE_D) < 0)
    fail("vmclearflags(full space) failed");

  dump_stage("after clearing A/D");
  show_flags("global value", (void *)&global_value, sizeof(global_value));
  show_flags("stack value", (void *)&stack_value, sizeof(stack_value));
  show_flags("stack array item", (void *)&stack_array[321], sizeof(stack_array[321]));
  show_flags("heap array", (void *)heap_array, heap_bytes);

  check_eq("global clear", vmcheckflags((void *)&global_value, sizeof(global_value), PTE_A | PTE_D), 0);
  check_eq("heap clear", vmcheckflags((void *)heap_array, heap_bytes, PTE_A | PTE_D), 0);

  rd = global_value;
  rd += stack_value;
  rd += stack_array[321];
  rd += heap_array[0];
  rd += heap_array[PGSIZE];
  rd += heap_array[2 * PGSIZE + 42];
  printf("read sum = %d\n", rd);

  dump_stage("after reading");
  show_flags("global value", (void *)&global_value, sizeof(global_value));
  show_flags("stack value", (void *)&stack_value, sizeof(stack_value));
  show_flags("stack array item", (void *)&stack_array[321], sizeof(stack_array[321]));
  show_flags("heap array", (void *)heap_array, heap_bytes);

  check_eq("global accessed after read", vmcheckflags((void *)&global_value, sizeof(global_value), PTE_A), 1);
  check_eq("global not dirty after read", vmcheckflags((void *)&global_value, sizeof(global_value), PTE_D), 0);
  check_eq("heap accessed after read", vmcheckflags((void *)heap_array, heap_bytes, PTE_A), 1);
  check_eq("heap not dirty after read", vmcheckflags((void *)heap_array, heap_bytes, PTE_D), 0);

  global_value += 10;
  stack_value += 10;
  stack_array[321] += 10;
  heap_array[0] += 10;
  heap_array[PGSIZE] += 10;
  heap_array[2 * PGSIZE + 42] += 10;

  dump_stage("after writing");
  show_flags("global value", (void *)&global_value, sizeof(global_value));
  show_flags("stack value", (void *)&stack_value, sizeof(stack_value));
  show_flags("stack array item", (void *)&stack_array[321], sizeof(stack_array[321]));
  show_flags("heap array", (void *)heap_array, heap_bytes);

  check_eq("global dirty after write", vmcheckflags((void *)&global_value, sizeof(global_value), PTE_D), 1);
  check_eq("heap dirty after write", vmcheckflags((void *)heap_array, heap_bytes, PTE_D), 1);

  if(sbrk(-heap_bytes) == SBRK_ERROR) fail("sbrk(-heap) failed");

  dump_stage("after free");
  show_flags("global value", (void *)&global_value, sizeof(global_value));
  show_flags("stack value", (void *)&stack_value, sizeof(stack_value));
  show_flags("stack array item", (void *)&stack_array[321], sizeof(stack_array[321]));
  check_eq("heap range rejected after free", vmcheckflags((void *)heap_array, heap_bytes, PTE_A), -1);

  printf("vmflags: OK\n");
  exit(0);
}
