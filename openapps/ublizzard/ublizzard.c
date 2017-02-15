#include "opendefs.h"
#include "ublizzard.h"
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
#define UBLIZZARDPERIOD  3000
/// my MAC address(8), ASN(5), total numTX (4 Bytes), numTxAck (4 Bytes) 
#define UBLIZZARDPAYLOADLEN      21

//=========================== variables =======================================
ublizzard_vars_t ublizzard_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void ublizzard_init() {
    ublizzard_vars.busySendingData      = FALSE;
	leds_debug_on();
	timerId_ublizzard    = opentimers_start(UBLIZZARDPERIOD,
                                               TIMER_PERIODIC,TIME_MS,
	                                           ublizzard_timer_cb);
}

void ublizzard_receive(OpenQueueEntry_t* request) {
	leds_debug_toggle();
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void ublizzard_timer_cb(opentimer_id_t id){
   scheduler_push_task(ublizzard_task_cb,TASKPRIO_COAP);
}

void ublizzard_task_cb() {
   OpenQueueEntry_t*    pkt;
   open_addr_t*         myadd64;
   uint32_t             numTxTotal;
   uint32_t             numTxAckTotal;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE){
       ublizzard_vars.busySendingData = FALSE;
       return;
   }

   if (ublizzard_vars.busySendingData==TRUE) {
      // don't continue if I'm still sending a previous data packet
      return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(timerId_ublizzard);
      return;
   }

   // create a Blizzard Report packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UBLIZZARD);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UBLIZZARD,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      //openqueue_freePacketBuffer(pkt);
      return;
   }
   // take ownership over that packet
   pkt->creator                   = COMPONENT_UBLIZZARD;
   pkt->owner                     = COMPONENT_UBLIZZARD;
   // Blizzard payload
   packetfunctions_reserveHeaderSize(pkt,UBLIZZARDPAYLOADLEN);

   myadd64                   = idmanager_getMyID(ADDR_64B);
   memcpy(&(pkt->payload[0]),myadd64->addr_64b,8);
   neighbors_get_retrial_statistics(&numTxTotal,&numTxAckTotal);
   memcpy(&(pkt->payload[8]),&numTxTotal,4);
   memcpy(&(pkt->payload[12]),&numTxAckTotal,4);
   ieee154e_getAsn(&(pkt->payload[16]));

   // metadata
   pkt->l4_protocol               = IANA_UDP;
   pkt->l4_destination_port       = WKP_UDP_BLIZZARD;
   pkt->l4_sourcePortORicmpv6Type = WKP_UDP_BLIZZARD;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_RootBlizzard,16);

   // send
   ublizzard_vars.busySendingData = TRUE;
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
      ublizzard_vars.busySendingData = FALSE;
   }

   return;
}

// To-Do: Modify openudp_sendDone() in openudp.c
void ublizzard_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   ublizzard_vars.busySendingData = FALSE;
   openqueue_freePacketBuffer(msg);
}

bool ublizzard_debugPrint() {
   return FALSE;
}

//=========================== private =========================================
