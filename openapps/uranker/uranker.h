#ifndef __URANKER_H
#define __URANKER_H

/**
\addtogroup AppUdp
\{
\addtogroup uranker
\{
*/

#include "openudp.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   udp_resource_desc_t desc;  ///< resource descriptor for this module, used to register at UDP stack
   uint8_t busySending;
   uint8_t busySendingRes;
   uint8_t backoff;
   uint8_t waitingRes;
   opentimers_id_t timerId_task;
   opentimers_id_t timerId_timeout;
   open_addr_t requestedNeighbor;
} uranker_vars_t;

//=========================== prototypes ======================================

void uranker_init(void);
void uranker_receive(OpenQueueEntry_t* msg);
void uranker_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void uranker_timer_cb(opentimers_id_t id);
void uranker_timeout_timer_cb(opentimers_id_t id);
void uranker_task_cb(void);
void uranker_task_timeout_cb(void);

/**
\}
\}
*/

#endif
