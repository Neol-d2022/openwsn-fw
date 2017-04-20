#ifndef __UTYPHOON_H
#define __UTYPHOON_H

/**
\addtogroup AppUdp
\{
\addtogroup utyphoon
\{
*/
#include "opentimers.h"
//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

static const uint8_t ipAddr_RootTyphoon[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
typedef struct {
   opentimer_id_t       timerId_utyphoon;
   uint32_t             dataCounter;
   bool                 busySendingData;           // TRUE when busy sending a data packet
} utyphoon_vars_t;

//=========================== prototypes ======================================

void utyphoon_init(void);
void utyphoon_receive(OpenQueueEntry_t* msg);
void utyphoon_sendDone(OpenQueueEntry_t* msg, owerror_t error);
bool utyphoon_debugPrint(void);
void utyphoon_timer_cb(opentimer_id_t id);
void utyphoon_task_cb(void);
uint32_t utyphoon_get_test_sensor_data(void);

/**
\}
\}
*/

#endif
