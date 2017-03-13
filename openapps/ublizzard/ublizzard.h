#ifndef __UBLIZZARD_H
#define __UBLIZZARD_H

/**
\addtogroup AppUdp
\{
\addtogroup ublizzard
\{
*/
#include "opentimers.h"
//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

static const uint8_t ipAddr_RootBlizzard[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

typedef struct {
   opentimer_id_t       timerId_ublizzard;
   bool                 busySendingData;           // TRUE when busy sending a data packet
} ublizzard_vars_t;

//=========================== prototypes ======================================

void ublizzard_init(void);
void ublizzard_receive(OpenQueueEntry_t* msg);
void ublizzard_sendDone(OpenQueueEntry_t* msg, owerror_t error);
bool ublizzard_debugPrint(void);
void ublizzard_timer_cb(opentimer_id_t id);
void ublizzard_task_cb(void);

/**
\}
\}
*/

#endif
