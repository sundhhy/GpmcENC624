/*
 * pbuf.h
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 */

#ifndef PBUF_H_
#define PBUF_H_

#include "net_config.h"
#include "osAbstraction.h"
#include "def.h"
typedef enum {
//  PBUF_TRANSPORT,
//  PBUF_IP,
  PBUF_LINK,
  PBUF_RAW
} pbuf_layer;

typedef enum {
  PBUF_RAM, /* pbuf data is stored in RAM */
  PBUF_ROM, /* pbuf data is stored in ROM */
  PBUF_REF, /* pbuf comes from the pbuf pool */
  PBUF_POOL /* pbuf payload refers to RAM */
} pbuf_type;




struct pbuf {
  /** next pbuf in singly linked pbuf chain */
  struct pbuf *next;

  /** pointer to the actual data in the buffer */
  void *payload;

  /**
   * total length of this buffer and all next buffers in chain
   * belonging to the same packet.
   *
   * For non-queue packet chains this is the invariant:
   * p->tot_len == p->len + (p->next? p->next->tot_len: 0)
   */
  u16_t tot_len;

  /** length of this buffer */
  u16_t len;

  /** pbuf_type as u8_t instead of enum to save space */
  u8_t /*pbuf_type*/ type;

  /** misc flags */
  u8_t flags;

  /**
   * the reference count always equals the number of pointers
   * that refer to this pbuf. This can be pointers from an application,
   * the stack itself, or pbuf->next pointers from a chain.
   */
   u16_t ref;
};
void
pbuf_init(void);

struct pbuf *
pbuf_alloc(pbuf_layer layer, u16_t length, pbuf_type type);
u8_t
pbuf_free(struct pbuf *p);

u8_t
pbuf_header(struct pbuf *p, s16_t header_size_increment);

err_t
pbuf_take(struct pbuf *buf, const void *dataptr, u16_t len);
u16_t
pbuf_memcmp(struct pbuf* p, u16_t offset, const void* s2, u16_t n);
u16_t
pbuf_strstr(struct pbuf* p, const char* substr);

#endif /* PBUF_H_ */
