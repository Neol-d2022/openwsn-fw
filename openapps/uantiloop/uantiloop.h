#ifndef __UANTILOOP_H
#define __UANTILOOP_H

/**
\addtogroup AppUdp
\{
\addtogroup uantiloop
\{
*/

#include "openudp.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   udp_resource_desc_t desc;  ///< resource descriptor for this module, used to register at UDP stack
   uint8_t busySending;
} uantiloop_vars_t;

//=========================== prototypes ======================================

void uantiloop_init(void);
void uantiloop_receive(OpenQueueEntry_t* msg);
void uantiloop_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void uantiloop_loopDetected(open_addr_t* sender);

/**
\}
\}
*/

#endif
