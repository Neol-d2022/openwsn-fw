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
#include <stdio.h>

//=========================== defines =========================================

/// inter-packet period (in ms)
#define UHURRICANEPERIOD  60000
/// code (1B), MyInfo (10 or 12B), Nbr1 (12B), Nbr2 (12B), Nbr3 (12B)
#define UHURRICANEPAYLOADLEN      49

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void uhurricane_init() {
	//uint8_t test[] = {0x00, 0x12, 0x4b, 0x00, 0x06, 0x0d, 0x84, 0xb4, \
	                                           0x00, 0x12, 0x4b, 0x00, 0x06, 0x0d, 0x86, 0x99};
	//neighbors_set2parents(test,2);

	timerId_uhurricane    = opentimers_start(UHURRICANEPERIOD,
                                               TIMER_PERIODIC,TIME_MS,
	                                           uhurricane_timer_cb);
	neighbors_set2parents(NULL,0);
}

void uhurricane_receive(OpenQueueEntry_t* request) {
	uint16_t          temp_l4_destination_port;
	OpenQueueEntry_t* reply;
	open_addr_t*      myadd64;

	reply = openqueue_getFreePacketBuffer(COMPONENT_UHURRICANE);
	if (reply==NULL) {
	   openserial_printError(
	      COMPONENT_UHURRICANE,
	      ERR_NO_FREE_PACKET_BUFFER,
	      (errorparameter_t)0,
	      (errorparameter_t)0
	   );
	   openqueue_freePacketBuffer(request); //clear the request packet as well
	   return;
	}

	reply->owner                         = COMPONENT_UHURRICANE;

	// reply with the same OpenQueueEntry_t
	reply->creator                       = COMPONENT_UHURRICANE;
	reply->l4_protocol                   = IANA_UDP;
	temp_l4_destination_port           = request->l4_destination_port;
	reply->l4_destination_port           = request->l4_sourcePortORicmpv6Type;
	reply->l4_sourcePortORicmpv6Type     = temp_l4_destination_port;
	reply->l3_destinationAdd.type        = ADDR_128B;

	// copy source to destination to echo.
	memcpy(&reply->l3_destinationAdd.addr_128b[0],&request->l3_sourceAdd.addr_128b[0],16);

	packetfunctions_reserveHeaderSize(reply,12);
	memset(reply->payload, 0, 12);
	myadd64 = idmanager_getMyID(ADDR_64B);
	memcpy(&reply->payload[0],myadd64->addr_64b,sizeof(open_addr_t));

	if ((openudp_send(reply))==E_FAIL) {
	   openqueue_freePacketBuffer(reply);
	}

	////

	if(request->length==16)
		neighbors_set2parents(&request->payload[8],1);
	else if(request->length==24)
		neighbors_set2parents(&request->payload[8],2);

	openqueue_freePacketBuffer(request);
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void uhurricane_timer_cb(opentimer_id_t id){
   scheduler_push_task(uhurricane_task_cb,TASKPRIO_COAP);
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

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(timerId_uhurricane);
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
   memcpy(&(pkt->payload[ 5]),&residualEnergy,sizeof(residualEnergy));
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
