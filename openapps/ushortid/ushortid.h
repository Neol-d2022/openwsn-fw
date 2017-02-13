#ifndef __USHORTID_H
#define __USHORTID_H

/**
\addtogroup AppUdp
\{
\addtogroup ushortid
\{
*/
//=========================== define ==========================================

//=========================== typedef =========================================

opentimer_id_t       timerId_ushortid;
opentimer_id_t       timerId_ushortid_timeout;
static const uint8_t ipAddr_RootShortID[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
typedef struct {
   uint8_t              desireAddr[8];
   bool                 busySendingData;           // TRUE when busy sending a data packet
   bool                 waitingRes;
} ushortid_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void ushortid_init(void);
void ushortid_receive(OpenQueueEntry_t* request);
void ushortid_timer_cb(opentimer_id_t id);
void ushortid_timeout_timer_cb(opentimer_id_t id);
void ushortid_task_cb(void);
void ushortid_sendDone(OpenQueueEntry_t* msg, owerror_t error);
/**
\}
\}
*/

#endif
