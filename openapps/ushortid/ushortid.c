/**
\brief An example CoAP application.
*/

#include "opendefs.h"
#include "openudp.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"
//#include "ADC_Channel.h"
#include "idmanager.h"
#include "IEEE802154E.h"
#include "icmpv6rpl.h"
#include "ushortid.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define USHORTIDPERIOD       4000
#define USHORTIDTIMEOUT_BASE 10000

#define USHORTIDPAYLOADLEN  10

//const uint8_t ushortid_path0[] = "ex";

//=========================== variables =======================================

ushortid_vars_t    ushortid_vars;
addrParents_vars_t addrParents_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void ushortid_init() {
   memset(&ushortid_vars,0,sizeof(ushortid_vars));
   ushortid_vars.timerId_ushortid = opentimers_create();
   if(uhurricane_vars.timerId_uhurricane == TOO_MANY_TIMERS_ERROR) {
   	//printf("[ERROR] %hu Cannot initialize ushort module: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
   	return;
   }
   opentimers_scheduleIn(ushortid_vars.timerId_ushortid, USHORTIDPERIOD + (openrandom_get16b() % USHORTIDPERIOD), TIME_MS, TIMER_PERIODIC, ushortid_timer_cb);
   ushortid_vars.backoff = (openrandom_get16b() & 0x07) + 0x01;
   
   // register at UDP stack
   ushortid_vars.desc.port              = WKP_UDP_SHORTID;
   ushortid_vars.desc.callbackReceive   = &ushortid_receive;
   ushortid_vars.desc.callbackSendDone  = &ushortid_sendDone;
   openudp_register(&ushortid_vars.desc);
}

//=========================== private =========================================

void ushortid_receive(OpenQueueEntry_t* request) {
    open_addr_t addr;
    uint16_t sid;
    uint8_t nidx;

    if(ushortid_vars.waitingRes==TRUE) {
        if(memcmp(request->payload+0,ushortid_vars.desireAddr,sizeof(ushortid_vars.desireAddr)) == 0) {
            //This is the addr we asked for
            opentimers_cancel(ushortid_vars.timerId_ushortid_timeout);
            opentimers_destroy(ushortid_vars.timerId_ushortid_timeout);
            sid  = ((uint8_t)request->payload[8]) << 8;
            sid += ((uint8_t)request->payload[9]) << 0;
            if(ushortid_vars.askingSelf==TRUE) {
                ushortid_vars.mysid = sid;
                //printf("[INFO] %hhu got reply %hu asking for short id of %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], sid, ushortid_vars.desireAddr[0], ushortid_vars.desireAddr[1], ushortid_vars.desireAddr[2], ushortid_vars.desireAddr[3], ushortid_vars.desireAddr[4], ushortid_vars.desireAddr[5], ushortid_vars.desireAddr[6], ushortid_vars.desireAddr[7]);
            }
            else {
                addr.type = ADDR_64B;
                memcpy(addr.addr_64b,ushortid_vars.desireAddr,LENGTH_ADDR64b);
                nidx = neighbors_addressToIndex(&addr);
                if (nidx < MAXNUMNEIGHBORS) {
                    neighbors_set_ushortid(nidx, sid);
                    //printf("[INFO] %hhu got reply %hu asking for short id of %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], sid, ushortid_vars.desireAddr[0], ushortid_vars.desireAddr[1], ushortid_vars.desireAddr[2], ushortid_vars.desireAddr[3], ushortid_vars.desireAddr[4], ushortid_vars.desireAddr[5], ushortid_vars.desireAddr[6], ushortid_vars.desireAddr[7]);
                }
                else {
                    // OOPS, addr is not neighbor nor self addr
                    //printf("[INFO] %hhu got reply %hu but for %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx, asking for short id of %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], sid, request->payload[0], request->payload[1], request->payload[2], request->payload[3], request->payload[4], request->payload[5], request->payload[6], request->payload[7], ushortid_vars.desireAddr[0], ushortid_vars.desireAddr[1], ushortid_vars.desireAddr[2], ushortid_vars.desireAddr[3], ushortid_vars.desireAddr[4], ushortid_vars.desireAddr[5], ushortid_vars.desireAddr[6], ushortid_vars.desireAddr[7]);
                }
            }
            memset(ushortid_vars.desireAddr,0,sizeof(ushortid_vars.desireAddr));
            ushortid_vars.askingSelf = FALSE;
            ushortid_vars.waitingRes = FALSE;
        }
    }
    openqueue_freePacketBuffer(request);
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void ushortid_timer_cb(opentimers_id_t id){
   scheduler_push_task(ushortid_task_cb,TASKPRIO_COAP);
}

void ushortid_timeout_timer_cb(opentimers_id_t id){
   if(ushortid_vars.waitingRes==TRUE) {
      ushortid_vars.waitingRes = FALSE; //Query timed out
      ushortid_vars.askingSelf = FALSE;
      //printf("[INFO] %hhu timed out asking for short id of %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ushortid_vars.desireAddr[0], ushortid_vars.desireAddr[1], ushortid_vars.desireAddr[2], ushortid_vars.desireAddr[3], ushortid_vars.desireAddr[4], ushortid_vars.desireAddr[5], ushortid_vars.desireAddr[6], ushortid_vars.desireAddr[7]);
      memset(ushortid_vars.desireAddr,0,sizeof(ushortid_vars.desireAddr));
      ushortid_vars.backoff = (openrandom_get16b() & 0x07) + 0x01;
   }
   opentimers_destroy(id);
}

void ushortid_task_cb() {
   open_addr_t          nadd64;
   OpenQueueEntry_t*    pkt;
   open_addr_t*         add64;
   uint16_t             shortID = 0;
   uint8_t              nidx;
   uint8_t              parentIdx;
   bool                 askingSelf;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE){
      ushortid_vars.busySendingData = FALSE;
      return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_destroy(ushortid_vars.timerId_ushortid);
      return;
   }

   // don't run when there is no parent
   if (icmpv6rpl_getPreferredParentIndex(&parentIdx) == FALSE) {
      return;
   }
   
   if (ushortid_vars.busySendingData==TRUE) {
      // don't continue if I'm still sending a previous data packet
      return;
   }
   
   // still waiting for a result
   if(ushortid_vars.waitingRes==TRUE) {
      return;
   }

   // still in backoff (i.e packet buffer full or collision happened)
   if(ushortid_vars.backoff != 0) {
      ushortid_vars.backoff = ushortid_vars.backoff - 1;
      return;
   }

   if(ushortid_vars.mysid == 0) {
      // I don't know my id, take care of it first
      askingSelf = TRUE;
      add64 = idmanager_getMyID(ADDR_64B);
   }
   else {
      // Ask for a neighbor's id
      askingSelf = FALSE;
      nidx = neighbors_nextNull_ushortid();
      if(nidx < MAXNUMNEIGHBORS) {
         neighbors_getNeighborEui64(&nadd64, ADDR_64B, nidx);
         add64 = &nadd64;
      }
      else {
         // All neighbors' id are known, or there are no neighbors
         return ;
      }
   }
   
   ushortid_vars.timerId_ushortid_timeout = opentimers_create();
   if(ushortid_vars.timerId_ushortid_timeout == TOO_MANY_TIMERS_ERROR) {
   	//printf("[ERROR] %hu Cannot send ushort request: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
   	return;
   }
   
   // Try to make a request
   ushortid_vars.busySendingData = TRUE;

   pkt = openqueue_getFreePacketBuffer(COMPONENT_USHORTID);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_USHORTID,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      ushortid_vars.busySendingData = FALSE;
      opentimers_destroy(ushortid_vars.timerId_ushortid_timeout);
      ushortid_vars.timerId_ushortid_timeout = TOO_MANY_TIMERS_ERROR;
      return;
   }
   // take ownership over that packet
   pkt->creator                   = COMPONENT_USHORTID;
   pkt->owner                     = COMPONENT_USHORTID;
   // payload
   packetfunctions_reserveHeaderSize(pkt,USHORTIDPAYLOADLEN);
   memcpy(&(pkt->payload[0]),add64->addr_64b,8);    
   (pkt->payload[8]) = (shortID & 0xFF00) >> 8;
   (pkt->payload[9]) = (shortID & 0x00FF) >> 0;
   
   // metadata
   pkt->l4_protocol               = IANA_UDP;
   pkt->l4_destination_port       = WKP_UDP_SHORTID;
   pkt->l4_sourcePortORicmpv6Type = WKP_UDP_SHORTID;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_RootShortID,16);

   // send
   ushortid_vars.askingSelf = askingSelf;
   memcpy(ushortid_vars.desireAddr,add64->addr_64b,sizeof(ushortid_vars.desireAddr));
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
      ushortid_vars.askingSelf = FALSE;
      memset(ushortid_vars.desireAddr,0,sizeof(ushortid_vars.desireAddr));
      ushortid_vars.backoff = (openrandom_get16b() & 0x07) + 0x01;
      ushortid_vars.busySendingData = FALSE;
      opentimers_destroy(ushortid_vars.timerId_ushortid_timeout);
      return;
   }

   //printf("[INFO] %hhu asking for short id of %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ushortid_vars.desireAddr[0], ushortid_vars.desireAddr[1], ushortid_vars.desireAddr[2], ushortid_vars.desireAddr[3], ushortid_vars.desireAddr[4], ushortid_vars.desireAddr[5], ushortid_vars.desireAddr[6], ushortid_vars.desireAddr[7]);
   return;
}

void ushortid_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   ushortid_vars.waitingRes = TRUE;
   ushortid_vars.busySendingData = FALSE;
   opentimers_scheduleIn(ushortid_vars.timerId_ushortid_timeout, USHORTIDTIMEOUT_BASE + (((icmpv6rpl_getMyDAGrank() + 255) >> 8) * SLOTFRAME_LENGTH * 15 * 4 * 2), TIME_MS, TIMER_ONESHOT, ushortid_timeout_timer_cb);
   openqueue_freePacketBuffer(msg);
}

uint16_t ushortid_myid(void) {
    return ushortid_vars.mysid;
}
