#include "opendefs.h"
#include "uantiloop.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"

//=========================== variables =======================================

uantiloop_vars_t uantiloop_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void uantiloop_init() {
   // clear local variables
   memset(&uantiloop_vars,0,sizeof(uantiloop_vars_t));

   // register at UDP stack
   uantiloop_vars.desc.port              = WKP_UDP_ANTILOOP;
   uantiloop_vars.desc.callbackReceive   = &uantiloop_receive;
   uantiloop_vars.desc.callbackSendDone  = &uantiloop_sendDone;
   openudp_register(&uantiloop_vars.desc);
}

void uantiloop_receive(OpenQueueEntry_t* request) {
   uint8_t parentIndex, senderIndex;

   printf("[INFO] %hu receive antiloop packet from %hu\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ((request->l2_nextORpreviousHop).addr_64b)[7]);
   if((senderIndex = neighbors_addressToIndex(&(request->l2_nextORpreviousHop))) < MAXNUMNEIGHBORS) {
      if(icmpv6rpl_getPreferredParentIndex(&parentIndex)) {
         if(senderIndex == parentIndex) {
            icmpv6rpl_notify_loopDetected();
         }
      }
   }
   openqueue_freePacketBuffer(request);
}

void uantiloop_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   uantiloop_vars.busySending -= 1;
   openqueue_freePacketBuffer(msg);
}

void uantiloop_loopDetected(open_addr_t* sender) {
   OpenQueueEntry_t* reply;
   
   if(neighbors_addressToIndex(sender) >= MAXNUMNEIGHBORS)
      return;
   
   if(uantiloop_vars.busySending >= 2)
      return;
   
   reply = openqueue_getFreePacketBuffer(COMPONENT_UANTILOOP);
   if (reply==NULL) {
      openserial_printError(
         COMPONENT_UANTILOOP,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   reply->owner                         = COMPONENT_UANTILOOP;
   reply->creator                       = COMPONENT_UANTILOOP;
   reply->l4_protocol                   = IANA_UDP;
   reply->l4_destination_port           = WKP_UDP_ANTILOOP;
   reply->l4_sourcePortORicmpv6Type     = WKP_UDP_ANTILOOP;
   reply->l3_destinationAdd.type        = ADDR_128B;
   
   packetfunctions_mac64bToIp128b(idmanager_getMyID(ADDR_PREFIX),sender,&reply->l3_destinationAdd);
   
   packetfunctions_reserveHeaderSize(reply,LENGTH_ADDR64b);
   memcpy(&reply->payload[0],idmanager_getMyID(ADDR_64B),LENGTH_ADDR64b);
   
   printf("[INFO] %hu send antiloop packet to %hu\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], (sender->addr_64b)[7]);
   uantiloop_vars.busySending += 1;
   if ((openudp_send(reply))==E_FAIL) {
      openqueue_freePacketBuffer(reply);
   }
}

//=========================== private =========================================
