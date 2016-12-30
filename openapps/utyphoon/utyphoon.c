#include "opendefs.h"
#include "utyphoon.h"
#include "openudp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "leds.h"
#include "neighbors.h"

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
	utyphoon_vars.busySendingData      = FALSE;
  leds_debug_on();
	timerId_utyphoon    = opentimers_start(UTYPHOONPERIOD,
                                         TIMER_PERIODIC,TIME_MS,
	                                       utyphoon_timer_cb);
}

void utyphoon_receive(OpenQueueEntry_t* request) {
	leds_debug_toggle();
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void utyphoon_timer_cb(opentimer_id_t id){
   scheduler_push_task(utyphoon_task_cb,TASKPRIO_COAP);
}

uint32_t get_test_sensor_data(){
   static uint32_t      data_4B = 0;
   data_4B++;
   return data_4B;
}

void utyphoon_task_cb() {
   OpenQueueEntry_t*    pkt;
   open_addr_t*         myadd64;
   uint32_t             sensorData;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE){
      utyphoon_vars.busySendingData = FALSE;
      return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(timerId_utyphoon);
      return;
   }
   
   sensorData = get_test_sensor_data();    // If network is busy, data will be discarded.
   
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
      openqueue_freePacketBuffer(pkt);
      return;
   }
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
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   utyphoon_vars.busySendingData = TRUE;

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
