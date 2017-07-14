#include "opendefs.h"
#include "uranker.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define URANKERPERIOD  (4 * SLOTFRAME_LENGTH * 15)
#define URANKERTIMEOUT (4 * SLOTFRAME_LENGTH * 15)

//=========================== variables =======================================

uranker_vars_t uranker_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void uranker_init() {
   // clear local variables
   memset(&uranker_vars,0,sizeof(uranker_vars_t));
   
   uranker_vars.timerId_task = opentimers_create();
   if(uranker_vars.timerId_task == TOO_MANY_TIMERS_ERROR) {
   	printf("[ERROR] %hu Cannot initialize uranker module: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
   	return;
   }
   opentimers_scheduleIn(uranker_vars.timerId_task, URANKERPERIOD + (openrandom_get16b() % (SLOTFRAME_LENGTH * 15)), TIME_MS, TIMER_PERIODIC, uranker_timer_cb);

   // register at UDP stack
   uranker_vars.desc.port              = WKP_UDP_RANKER;
   uranker_vars.desc.callbackReceive   = &uranker_receive;
   uranker_vars.desc.callbackSendDone  = &uranker_sendDone;
   openudp_register(&uranker_vars.desc);
   
   uranker_vars.backoff = 16 + (openrandom_get16b() & 0x07);
}

void uranker_receive(OpenQueueEntry_t* request) {
   uint16_t          temp_l4_destination_port;
   OpenQueueEntry_t* reply;
   dagrank_t rank;
   uint8_t retType = 1, retCode = 1, hasToReply = 0, i;
   
   if(request->length > 0) {
      if(request->payload[0] == 0) {
         // We received a request
         retType = 1; // Type = response
         retCode = 0; // Code = Success
         
         hasToReply = 1;
         printf("[INFO] %hu receive RANKER request packet from %hu\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ((request->l2_nextORpreviousHop).addr_64b)[7]);
      }
      else if(request->payload[0] == 1) {
         // This is a response
         //printf("[INFO] %hu received a rank response\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
         if(uranker_vars.waitingRes == 1) {
            if(request->length > 1) {
               if(request->payload[1] == 0) {
                  if(packetfunctions_sameAddress(&uranker_vars.requestedNeighbor, &(request->l2_nextORpreviousHop))) {
                     if(uranker_vars.timerId_timeout != TOO_MANY_TIMERS_ERROR) {
                        opentimers_cancel(uranker_vars.timerId_timeout);
                        opentimers_destroy(uranker_vars.timerId_timeout);
                        uranker_vars.timerId_timeout = TOO_MANY_TIMERS_ERROR;
                     }
                     if(request->length == 2 + sizeof(rank)) {
                        rank = (uint16_t)((request->payload[2] << 8) + request->payload[3]);
                        i = neighbors_addressToIndex(&uranker_vars.requestedNeighbor);
                        if(i < MAXNUMNEIGHBORS) {
                           if(rank == DEFAULTDAGRANK) {
                              uranker_vars.backoff = (openrandom_get16b() & 0x03) + 4;
                           }
                           neighbors_setNeighborRank(i, rank);
                           printf("[INFO] %hu receive RANKER packet from %hu, rank = %u\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ((request->l2_nextORpreviousHop).addr_64b)[7], (unsigned int)rank);
                        }
                        else {
                           // Not in neighbor table!
                           //printf("[INFO] %hu Not in neighbor table!\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
                        }
                     }
                     else {
                        // Protocol Error
                        //printf("[INFO] %hu Protocol Error(request->length != 2 + sizeof(rank))\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
                     }
                     uranker_vars.waitingRes = 0;
                     uranker_vars.requestedNeighbor.type = ADDR_NONE;
                  }
                  else {
                     // Unexpected reponse from this neighbor
                     //printf("[INFO] %hu Unexpected reponse from this neighbor\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
                  }
               }
               else {
                  // Peer reported an error
                  //printf("[INFO] %hu receive RANKER packet from %hu, errorCode = %u\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ((request->l2_nextORpreviousHop).addr_64b)[7], (unsigned int)request->payload[1]);
               }
            }
            else {
               // Protocol error (Malformed reponse)
               //printf("[INFO] %hu Protocol error(request->length <= 1)\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
            }
         }
         else {
            // Unexpected reponse
            //printf("[INFO] %hu Unexpected reponse\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
         }
      }
      else {
         // Protocol error
         //printf("[INFO] %hu Protocol error(Type unkonwn)\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
      }
   }
   else {
      // Protocol error
      //printf("[INFO] %hu Protocol error(request->length == 0)\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
   }
   
   if(hasToReply == 0) {
      openqueue_freePacketBuffer(request);
      return;
   }
   
   if(uranker_vars.busySendingRes >= 2) {
      openqueue_freePacketBuffer(request);
      return;
   }
   
   reply = openqueue_getFreePacketBuffer(COMPONENT_URANKER_RES);
   if (reply==NULL) {
      openserial_printError(
         COMPONENT_URANKER,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      openqueue_freePacketBuffer(request); //clear the request packet as well
      return;
   }
   
   reply->owner                         = COMPONENT_URANKER_RES;
   
   // reply with the same OpenQueueEntry_t
   reply->creator                       = COMPONENT_URANKER_RES;
   reply->l4_protocol                   = IANA_UDP;
   temp_l4_destination_port             = request->l4_destination_port;
   reply->l4_destination_port           = request->l4_sourcePortORicmpv6Type;
   reply->l4_sourcePortORicmpv6Type     = temp_l4_destination_port;
   reply->l3_destinationAdd.type        = ADDR_128B;
   
   // copy source to destination.
   memcpy(&reply->l3_destinationAdd.addr_128b[0],&request->l3_sourceAdd.addr_128b[0],16);
   
   if(retCode == 0) {
      packetfunctions_reserveHeaderSize(reply, sizeof(retType) + sizeof(retCode) + sizeof(rank));
      reply->payload[0] = retType;
      reply->payload[1] = retCode;
      rank = icmpv6rpl_getMyDAGrank();
      reply->payload[2] = (uint8_t)(rank >> 8);
      reply->payload[3] = (uint8_t)(rank & 0xFF);
   }
   else {
      packetfunctions_reserveHeaderSize(reply, sizeof(retType) + sizeof(retCode));
      reply->payload[0] = retType;
      reply->payload[1] = retCode;
   }
   
   openqueue_freePacketBuffer(request);
   if ((openudp_send(reply))==E_FAIL) {
      openqueue_freePacketBuffer(reply);
      return;
   }
   
   uranker_vars.busySendingRes += 1;
}

void uranker_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   if(msg->creator == COMPONENT_URANKER) {
      // Request
      uranker_vars.busySending -= 1;
      uranker_vars.waitingRes = 1;
      opentimers_scheduleIn(uranker_vars.timerId_timeout, URANKERTIMEOUT, TIME_MS, TIMER_ONESHOT, uranker_timeout_timer_cb);
   }
   else if(msg->creator == COMPONENT_URANKER_RES) {
      // Reply
      uranker_vars.busySendingRes -= 1;
   }
   else {
      printf("[INFO] %hu Error in uranker sendDone (%d)\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], (int)msg->creator);
   }
   openqueue_freePacketBuffer(msg);
}

void uranker_timer_cb(opentimers_id_t id) {
   scheduler_push_task(uranker_task_cb,TASKPRIO_COAP);
}

void uranker_timeout_timer_cb(opentimers_id_t id) {
   //scheduler_push_task(uranker_task_timeout_cb,TASKPRIO_COAP);
   uranker_vars.waitingRes = 0;
   uranker_vars.requestedNeighbor.type = ADDR_NONE;
   uranker_vars.timerId_timeout = TOO_MANY_TIMERS_ERROR;
   opentimers_destroy(id);
   printf("[INFO] %hu time out requesting rank\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
}

void uranker_task_cb(void) {
   OpenQueueEntry_t* request;
   open_addr_t neighbor;
   dagrank_t rank;
   uint8_t neighborIndex;
   
   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_destroy(uranker_vars.timerId_task);
      return;
   }
   
   if(uranker_vars.busySending >= 2)
      return;
   
   if(uranker_vars.backoff >  0) {
      uranker_vars.backoff -= 1;
      return;
   }
   
   for(neighborIndex=0;neighborIndex<MAXNUMNEIGHBORS;neighborIndex+=1) {
      if(neighbors_isStableNeighborByIndex(neighborIndex)) {
         rank = neighbors_getNeighborRank(neighborIndex);
         if(rank == DEFAULTDAGRANK) {
            neighbors_getNeighborEui64(&neighbor, ADDR_64B, neighborIndex);
            break;
         }
      }
   }
   
   if(neighborIndex >= MAXNUMNEIGHBORS)
      return;
   
   uranker_vars.timerId_timeout = opentimers_create();
   if(uranker_vars.timerId_timeout == TOO_MANY_TIMERS_ERROR) {
      printf("[ERROR] %hu Cannot send uranker request: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
      return;
   }
   
   request = openqueue_getFreePacketBuffer(COMPONENT_URANKER);
   if (request==NULL) {
      openserial_printError(
         COMPONENT_URANKER,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      opentimers_destroy(uranker_vars.timerId_timeout);
      uranker_vars.timerId_timeout = TOO_MANY_TIMERS_ERROR;
      return;
   }
   
   request->owner                         = COMPONENT_URANKER;
   request->creator                       = COMPONENT_URANKER;
   request->l4_protocol                   = IANA_UDP;
   request->l4_destination_port           = WKP_UDP_RANKER;
   request->l4_sourcePortORicmpv6Type     = WKP_UDP_RANKER;
   request->l3_destinationAdd.type        = ADDR_128B;
   
   packetfunctions_mac64bToIp128b(idmanager_getMyID(ADDR_PREFIX),&neighbor,&request->l3_destinationAdd);
   
   packetfunctions_reserveHeaderSize(request, sizeof(uint8_t));
   request->payload[0] = 0; // Type = reuqest
   
   printf("[INFO] %hu send rank request packet to %hu\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], (neighbor.addr_64b)[7]);
   
   if ((openudp_send(request))==E_FAIL) {
      openqueue_freePacketBuffer(request);
      opentimers_destroy(uranker_vars.timerId_timeout);
      uranker_vars.timerId_timeout = TOO_MANY_TIMERS_ERROR;
      return;
   }
   uranker_vars.busySending += 1;
   memcpy(&uranker_vars.requestedNeighbor,&neighbor, sizeof(uranker_vars.requestedNeighbor));
}

void uranker_task_timeout_cb(void) {
   uranker_vars.waitingRes = 0;
   uranker_vars.requestedNeighbor.type = ADDR_NONE;
}

//=========================== private =========================================
