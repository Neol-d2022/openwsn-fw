#include "opendefs.h"
#include "uhurricane.h"
#include "openudp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "leds.h"
#include "neighbors.h"
#include "icmpv6rpl.h"
#include "ushortid.h"
#include "icmpv6rpl.h"
#include <stdio.h>

//=========================== defines =========================================

/// inter-packet period (in ms)
#define UHURRICANEPERIOD  60000
/// code (1B), MyInfo (10 or 12B), Nbr1 (12B), Nbr2 (12B), Nbr3 (12B)
#define UHURRICANEPAYLOADLEN      49

//=========================== variables =======================================

uhurricane_vars_t uhurricane_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void uhurricane_init() {
   uhurricane_vars.timerId_uhurricane = opentimers_create();
   if(uhurricane_vars.timerId_uhurricane == TOO_MANY_TIMERS_ERROR) {
   	//printf("[ERROR] %hu Cannot initialize uhurricane module: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
   	return;
   }
   uhurricane_vars.timerId_uhurricane_timeout = TOO_MANY_TIMERS_ERROR;
   
   opentimers_scheduleIn(uhurricane_vars.timerId_uhurricane, UHURRICANEPERIOD, TIME_MS, TIMER_PERIODIC, uhurricane_timer_cb);
   neighbors_set2parents(NULL,0);
   
   // register at UDP stack
   uhurricane_vars.desc.port              = WKP_UDP_HURRICANE;
   uhurricane_vars.desc.callbackReceive   = &uhurricane_receive;
   uhurricane_vars.desc.callbackSendDone  = &uhurricane_sendDone;
   openudp_register(&uhurricane_vars.desc);
}

void uhurricane_receive(OpenQueueEntry_t* request) {
   if(uhurricane_vars.timerId_uhurricane_timeout != TOO_MANY_TIMERS_ERROR) {
      opentimers_cancel(uhurricane_vars.timerId_uhurricane_timeout);
   }
   else {
      uhurricane_vars.timerId_uhurricane_timeout = opentimers_create();
      if(uhurricane_vars.timerId_uhurricane_timeout == TOO_MANY_TIMERS_ERROR) {
         //printf("[ERROR] %hu Cannot process uhurricane response: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
         openqueue_freePacketBuffer(request);
         return;
      }
   }

   if(request->length==16)
      neighbors_set2parents(&request->payload[8],1);
   else if(request->length==24)
      neighbors_set2parents(&request->payload[8],2);
   
   //printf("[INFO] %hu Received routing rule(%d). Next hop: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], request->length, request->payload[8], request->payload[9], request->payload[10], request->payload[11], request->payload[12], request->payload[13], request->payload[14], request->payload[15]);
   openqueue_freePacketBuffer(request);
   
   opentimers_scheduleIn(uhurricane_vars.timerId_uhurricane_timeout, UHURRICANEPERIOD, TIME_MS, TIMER_ONESHOT, uhurricane_timeout_cb);
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void uhurricane_timer_cb(opentimers_id_t id) {
   scheduler_push_task(uhurricane_task_cb,TASKPRIO_COAP);
}

void uhurricane_timeout_reset_cb() {
   neighbors_set2parents(NULL,0);
}

void uhurricane_timeout_cb(opentimers_id_t id){
   uhurricane_vars.timerId_uhurricane_timeout = TOO_MANY_TIMERS_ERROR;
   opentimers_destroy(id);
   scheduler_push_task(uhurricane_timeout_reset_cb,TASKPRIO_COAP);
   //uhurricane_timeout_reset_cb();
}

void uhurricane_task_cb() {
   uint8_t              buf[64];
   OpenQueueEntry_t*    pkt;
   uint8_t              numNeighbor;
   uint8_t              code;
   dagrank_t            rank;
   uint16_t             residualEnergy;
   uint16_t             myid;
   uint8_t              uhurricanePayloadLen;
   uint8_t              neighborLen;
   uint8_t              parentIdx;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_destroy(uhurricane_vars.timerId_uhurricane);
      return;
   }

   // don't run when there is no parent
   if (icmpv6rpl_getPreferredParentIndex(&parentIdx) == FALSE) {
      return;
   }

   numNeighbor = neighbors_getNumNeighbors();
   if(numNeighbor==0) return;
   myid        = ushortid_myid();
   if(myid == 0) return;

   // create a Hurricane Report packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UHURRICANE);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UHURRICANE,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      //openqueue_freePacketBuffer(pkt);
      return;
   }
   // take ownership over that packet
   pkt->creator                   = COMPONENT_UHURRICANE;
   pkt->owner                     = COMPONENT_UHURRICANE;
   // Hurricane payload
   neighbors_get3parents(buf, &neighborLen);
   //uhurricanePayloadLen = UHURRICANEPAYLOADLEN - (3-numNeighbor)*12;
   uhurricanePayloadLen = 7 + (neighborLen*6);
   packetfunctions_reserveHeaderSize(pkt,uhurricanePayloadLen);

   code                      = 16 + numNeighbor;
   rank                      = icmpv6rpl_getMyDAGrank();
   residualEnergy            = 100;

   memcpy(&(pkt->payload[ 0]),&code,sizeof(code));
   (pkt->payload[1]) = (myid & 0xFF00) >> 8;
   (pkt->payload[2]) = (myid & 0x00FF) >> 0;
   (pkt->payload[3]) = (rank & 0xFF00) >> 8;
   (pkt->payload[4]) = (rank & 0x00FF) >> 0;
   (pkt->payload[5]) = (residualEnergy & 0xFF00) >> 8;
   (pkt->payload[6]) = (residualEnergy & 0x00FF) >> 0;
   memcpy(&(pkt->payload[ 7]),buf,neighborLen*6);

   // metadata
   pkt->l4_protocol               = IANA_UDP;
   pkt->l4_destination_port       = WKP_UDP_HURRICANE;
   pkt->l4_sourcePortORicmpv6Type = WKP_UDP_HURRICANE;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_RootHurricane,16);

   // send
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   else {
      //printf("[INFO] %hu reported %u neighbors\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], neighborLen);
   }

   //leds_debug_off();

   return;
}

void uhurricane_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

bool uhurricane_debugPrint() {
   return FALSE;
}

//=========================== private =========================================
