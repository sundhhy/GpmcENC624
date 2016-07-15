/*
 * raw.h
 *
 *  Created on: 2016-7-14
 *      Author: Administrator
 */

#ifndef RAW_H_
#define RAW_H_
#include <stdint.h>

struct raw_pcb {
  /* Common members of all PCB types */
//  IP_PCB;

  struct raw_pcb *next;

  uint8_t protocol;

  /** receive callback function */
//  raw_recv_fn recv;
  /* user-supplied argument for the recv callback */
  void *recv_arg;
};

#endif /* RAW_H_ */
