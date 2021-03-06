#include "opendefs.h"
#include "utyphoon.h"
#include "openudp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "neighbors.h"
#include "icmpv6rpl.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define UTYPHOONPERIOD  3000
/// my MAC address(8 bytes), seqNo(4 bytes), ASN(5 bytes)
#define UTYPHOONPAYLOADLEN      17

//=========================== variables =======================================
utyphoon_vars_t utyphoon_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void utyphoon_init() {
        utyphoon_vars.data_4B = 0;
	utyphoon_vars.busySendingData      = FALSE;
	utyphoon_vars.timerId_utyphoon     = opentimers_create();
        if(utyphoon_vars.timerId_utyphoon == TOO_MANY_TIMERS_ERROR)
        {
                return;
        }
        opentimers_scheduleIn(utyphoon_vars.timerId_utyphoon, UTYPHOONPERIOD, TIME_MS, TIMER_PERIODIC, utyphoon_timer_cb);
        utyphoon_vars.desc.port              = WKP_UDP_TYPHOON;
        utyphoon_vars.desc.callbackReceive   = &utyphoon_receive;
        utyphoon_vars.desc.callbackSendDone  = &utyphoon_sendDone;
        openudp_register(&utyphoon_vars.desc);
}

void utyphoon_receive(OpenQueueEntry_t* request) {
  openqueue_freePacketBuffer(request);
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void utyphoon_timer_cb(opentimers_id_t id){
   scheduler_push_task(utyphoon_task_cb,TASKPRIO_COAP);
}

uint32_t get_test_sensor_data(void){
   (utyphoon_vars.data_4B)++;
   return utyphoon_vars.data_4B;
}

void utyphoon_task_cb() {
   OpenQueueEntry_t*    pkt;
   open_addr_t*         myadd64;
   uint32_t             sensorData;
   uint8_t              parentIdx;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE){
      utyphoon_vars.busySendingData = FALSE;
      return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_cancel(utyphoon_vars.timerId_utyphoon);
      opentimers_destroy(utyphoon_vars.timerId_utyphoon);
      return;
   }

   // don't run when there is no parent
   if (icmpv6rpl_getPreferredParentIndex(&parentIdx) == FALSE) {
      return;
   }
   
   if (utyphoon_vars.busySendingData==TRUE) {
      // don't continue if I'm still sending a previous data packet
      return;
   }

   // create a Typhoon Report packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UTYPHOON);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UTYPHOON,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      //openqueue_freePacketBuffer(pkt);
      return;
   }
   sensorData = get_test_sensor_data();
   // take ownership over that packet
   pkt->creator                   = COMPONENT_UTYPHOON;
   pkt->owner                     = COMPONENT_UTYPHOON;
   // Typhoon payload
   packetfunctions_reserveHeaderSize(pkt,UTYPHOONPAYLOADLEN);
   myadd64                        = idmanager_getMyID(ADDR_64B);
   memcpy(&(pkt->payload[0]),myadd64->addr_64b,8);    
   memcpy(&(pkt->payload[8]),&sensorData,4);   
   ieee154e_getAsn(&(pkt->payload[12]));
   
   // metadata
   pkt->l4_protocol               = IANA_UDP;
   pkt->l4_destination_port       = WKP_UDP_TYPHOON;
   pkt->l4_sourcePortORicmpv6Type = WKP_UDP_TYPHOON;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_RootTyphoon,16);

   // send
   utyphoon_vars.busySendingData = TRUE;
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
      utyphoon_vars.busySendingData = FALSE;
   }

   return;
}

// To-Do: Modify openudp_sendDone() in openudp.c
void utyphoon_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   utyphoon_vars.busySendingData = FALSE;
   openqueue_freePacketBuffer(msg);
}

bool utyphoon_debugPrint() {
   return FALSE;
}

//=========================== private =========================================
