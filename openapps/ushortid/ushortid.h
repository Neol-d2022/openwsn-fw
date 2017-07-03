#ifndef __USHORTID_H
#define __USHORTID_H

/**
\addtogroup AppUdp
\{
\addtogroup ushortid
\{
*/

#include "openudp.h"

//=========================== define ==========================================

//=========================== typedef =========================================


static const uint8_t ipAddr_RootShortID[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
typedef struct {
   udp_resource_desc_t   desc;  ///< resource descriptor for this module, used to register at UDP stack
   opentimers_id_t       timerId_ushortid;
   opentimers_id_t       timerId_ushortid_timeout;
   uint16_t              mysid;
   uint8_t               desireAddr[8];
   uint8_t               backoff;
   bool                  busySendingData;           // TRUE when busy sending a data packet
   bool                  waitingRes;
   bool                  askingSelf;
} ushortid_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void ushortid_init(void);
void ushortid_receive(OpenQueueEntry_t* request);
void ushortid_timer_cb(opentimers_id_t id);
void ushortid_timeout_timer_cb(opentimers_id_t id);
void ushortid_task_cb(void);
void ushortid_sendDone(OpenQueueEntry_t* msg, owerror_t error);
uint16_t ushortid_myid(void);
/**
\}
\}
*/

#endif
