#ifndef __UHURRICANE_H
#define __UHURRICANE_H

/**
\addtogroup AppUdp
\{
\addtogroup uhurricane
\{
*/
#include "opentimers.h"
//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================
opentimer_id_t       timerId_uhurricane;
static const uint8_t ipAddr_RootHurricane[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

//=========================== prototypes ======================================

void uhurricane_init(void);
void uhurricane_receive(OpenQueueEntry_t* msg);
void uhurricane_sendDone(OpenQueueEntry_t* msg, owerror_t error);
bool uhurricane_debugPrint(void);
void uhurricane_timer_cb(opentimer_id_t id);
void uhurricane_task_cb(void);

/**
\}
\}
*/

#endif
