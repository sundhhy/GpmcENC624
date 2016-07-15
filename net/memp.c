/*
 * memp.c
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 *
 *     memp_memory： 一个能够把全部的缓冲都包含的数组
 *     memp_sizes： 每种类型的缓冲的长度
 *     memp_num：	每种类型的缓冲 的数量
 *     memp_tab：	指向各自缓存区的第一个空闲内存的指针
 *
 *	分配内存时，将memp_tab指向的那片内存分配出去，然后memp_tab指向下一片内存。
 *	如果memp_tab指向的内存为NULL,说明被分配完了
 *
 *	释放内存时，将被释放内存的下一片内采取指向memp_tab，memp_tab指向被释放的内存。
 *
 *
 * 就是链表操作的过程。分配内存是从链表中取删除节点，释放内存是把前面被删除的节点加入到链表头
 *初始化就是把这个链表建立起来
 *
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include "memp.h"
#include "mem.h"
#include "raw.h"
#include <assert.h>
#include "osAbstraction.h"
#include "pbuf.h"



pthread_mutex_t Mem_Pool_mutex = PTHREAD_MUTEX_INITIALIZER;


struct memp {
  struct memp *next;
#if MEMP_OVERFLOW_CHECK
  const char *file;
  int line;
#endif /* MEMP_OVERFLOW_CHECK */
};


#if MEMP_OVERFLOW_CHECK

#else  /* MEMP_OVERFLOW_CHECK */
#define MEMP_SIZE           0
#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x))

#endif  /* MEMP_OVERFLOW_CHECK */

const uint16_t memp_sizes[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  LWIP_MEM_ALIGN_SIZE(size),
#include "memp_std.h"

};

static const uint16_t memp_num[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  (num),
#include "memp_std.h"

};

#ifdef MEM_DEBUG
static const char *memp_desc[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  (desc),
#include "memp_std.h"

};
#endif

static uint8_t memp_memory[MEM_ALIGNMENT - 1
#define LWIP_MEMPOOL(name,num,size,desc) +( (num) * (MEMP_SIZE + MEMP_ALIGN_SIZE(size) ) )
#include "memp_std.h"
];

/* 各种缓冲池中的第一个空闲pool的指针  */
static struct memp *memp_tab[MEMP_MAX];







/**
 * Initialize this module.
 *
 * Carves out memp_memory into linked lists for each pool-type.
 */
void
memp_init(void)
{
  struct memp *memp;
  uint16_t i, j;



  memp = (struct memp *)LWIP_MEM_ALIGN(memp_memory);

  /* for every pool: */
  for (i = 0; i < MEMP_MAX; ++i) {
    memp_tab[i] = NULL;
    /* create a linked list of memp elements */
    for (j = 0; j < memp_num[i]; ++j) {
      memp->next = memp_tab[i];
      memp_tab[i] = memp;
      memp = (struct memp *)(void *)((uint8_t *)memp + MEMP_SIZE + memp_sizes[i]
#if MEMP_OVERFLOW_CHECK
        + MEMP_SANITY_REGION_AFTER_ALIGNED
#endif
      );
    }
  }
#if MEMP_OVERFLOW_CHECK
  memp_overflow_init();
  /* check everything a first time to see if it worked */
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK */
}

/**
 * Get an element from a specific pool.
 *
 * @param type the pool to get an element from
 *
 * the debug version has two more parameters:
 * @param file file name calling this function
 * @param line number of line where this function is called
 *
 * @return a pointer to the allocated memory or a NULL pointer on error
 */
void *memp_malloc(memp_t type)
{
  struct memp *memp;
//  todo 增加临界保护机制
  SYS_ARCH_DECL_PROTECT(Mem_Pool_mutex);

  assert( type < MEMP_MAX);

  SYS_ARCH_PROTECT(Mem_Pool_mutex);

  memp = memp_tab[type];

  if (memp != NULL) {
    memp_tab[type] = memp->next;
#if MEMP_OVERFLOW_CHECK
    memp->next = NULL;
    memp->file = file;
    memp->line = line;
#endif /* MEMP_OVERFLOW_CHECK */

    assert( ((mem_ptr_t)memp % MEM_ALIGNMENT) == 0);
    memp = (struct memp*)(void *)((uint8_t*)memp + MEMP_SIZE);
  }

  SYS_ARCH_UNPROTECT(Mem_Pool_mutex);

  return memp;
}


/**
 * Put an element back into its pool.
 *
 * @param type the pool where to put mem
 * @param mem the memp element to free
 */
void
memp_free(memp_t type, void *mem)
{
  struct memp *memp;
  SYS_ARCH_DECL_PROTECT(Mem_Pool_mutex);

  if (mem == NULL) {
    return;
  }


  assert( ((mem_ptr_t)mem % MEM_ALIGNMENT) == 0);
  memp = (struct memp *)(void *)((u8_t*)mem - MEMP_SIZE);

  SYS_ARCH_PROTECT(Mem_Pool_mutex);
#if MEMP_OVERFLOW_CHECK
#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#else
  memp_overflow_check_element_overflow(memp, type);
  memp_overflow_check_element_underflow(memp, type);
#endif /* MEMP_OVERFLOW_CHECK >= 2 */
#endif /* MEMP_OVERFLOW_CHECK */


  memp->next = memp_tab[type];
  memp_tab[type] = memp;



  SYS_ARCH_UNPROTECT(Mem_Pool_mutex);
}
