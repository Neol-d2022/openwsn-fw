#ifndef __UPROBER_H
#define __UPROBER_H

/**
\addtogroup AppUdp
\{
\addtogroup uprober
\{
*/

#include "openudp.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   udp_resource_desc_t desc;  ///< resource descriptor for this module, used to register at UDP stack
   uint8_t busySending;
   opentimers_id_t timerId_task;
} uprober_vars_t;

//=========================== prototypes ======================================

void uprober_init(void);
void uprober_receive(OpenQueueEntry_t* msg);
void uprober_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void uprober_timer_cb(opentimers_id_t id);
void uprober_task_cb(void);

/**
\}
\}
*/

#endif
