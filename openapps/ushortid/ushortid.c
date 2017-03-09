/**
\brief An example CoAP application.
*/

#include "opendefs.h"
#include "ushortid.h"
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

//=========================== defines =========================================

/// inter-packet period (in ms)
#define USHORTIDPERIOD       1000
#define USHORTIDTIMEOUT     10000

#define USHORTIDPAYLOADLEN  10

//const uint8_t ushortid_path0[] = "ex";

//=========================== variables =======================================

ushortid_vars_t ushortid_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void ushortid_init() {
   memset(&ushortid_vars,0,sizeof(ushortid_vars));
   timerId_ushortid          = opentimers_start(USHORTIDPERIOD,
                                                TIMER_PERIODIC,TIME_MS,
                                                ushortid_timer_cb);
   timerId_ushortid_timeout  = opentimers_start(USHORTIDTIMEOUT,
                                                TIMER_ONESHOT,TIME_MS,
                                                ushortid_timeout_timer_cb);
   opentimers_stop(timerId_ushortid_timeout);
}

//=========================== private =========================================

void ushortid_receive(OpenQueueEntry_t* request) {
    open_addr_t addr;
    uint16_t sid;
    uint8_t nidx;

    if(ushortid_vars.waitingRes==TRUE) {
        if(memcmp(request->payload+0,ushortid_vars.desireAddr,sizeof(ushortid_vars.desireAddr)) == 0) {
            //This is the addr we asked for
            ushortid_vars.waitingRes = FALSE;
            opentimers_stop(timerId_ushortid_timeout);
            sid  = ((uint8_t)request->payload[8]) << 8;
            sid += ((uint8_t)request->payload[9]) << 0;
            if(ushortid_vars.askingSelf==TRUE) {
                ushortid_vars.mysid = sid;
            }
            else {
                addr.type = ADDR_64B;
                memcpy(addr.addr_64b,ushortid_vars.desireAddr,LENGTH_ADDR64b);
                nidx = neighbors_addressToIndex(&addr);
                if (nidx < MAXNUMNEIGHBORS) {
                    neighbors_set_ushortid(nidx, sid);
                }
                else {
                    // OOPS, addr is not neighbor nor self addr
                }
            }
            memset(ushortid_vars.desireAddr,0,sizeof(ushortid_vars.desireAddr));
            ushortid_vars.askingSelf = FALSE;
        }
    }
	openqueue_freePacketBuffer(request);
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void ushortid_timer_cb(opentimer_id_t id){
   scheduler_push_task(ushortid_task_cb,TASKPRIO_COAP);
}

void ushortid_timeout_timer_cb(opentimer_id_t id){
   if(ushortid_vars.waitingRes==TRUE) {
      ushortid_vars.waitingRes = FALSE; //Query timed out
      ushortid_vars.askingSelf = FALSE;
      memset(ushortid_vars.desireAddr,0,sizeof(ushortid_vars.desireAddr));
   }
}

void ushortid_task_cb() {
   open_addr_t          nadd64;
   OpenQueueEntry_t*    pkt;
   open_addr_t*         add64;
   uint16_t             shortID = 0;
   uint8_t              nidx;
   bool                 askingSelf;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE){
      ushortid_vars.busySendingData = FALSE;
      return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(timerId_ushortid);
      return;
   }
   
   if (ushortid_vars.busySendingData==TRUE) {
      // don't continue if I'm still sending a previous data packet
      return;
   }

   if(ushortid_vars.waitingRes==TRUE) {
      return;
   }

   if(ushortid_vars.mysid == 0) {
      askingSelf = TRUE;
      add64 = idmanager_getMyID(ADDR_64B);
   }
   else {
      askingSelf = FALSE;
      nidx = neighbors_nextNull_ushortid();
      if(nidx < MAXNUMNEIGHBORS) {
         neighbors_getNeighborEui64(&nadd64, ADDR_64B, nidx);
         add64 = &nadd64;
      }
      else return ;
   }
   

   pkt = openqueue_getFreePacketBuffer(COMPONENT_USHORTID);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_USHORTID,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
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
   ushortid_vars.busySendingData = TRUE;
   ushortid_vars.askingSelf = askingSelf;
   memcpy(ushortid_vars.desireAddr,add64->addr_64b,sizeof(ushortid_vars.desireAddr));
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
      ushortid_vars.busySendingData = FALSE;
      ushortid_vars.askingSelf = FALSE;
      memset(ushortid_vars.desireAddr,0,sizeof(ushortid_vars.desireAddr));
   }

   return;
}

void ushortid_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   ushortid_vars.busySendingData = FALSE;
   ushortid_vars.waitingRes = TRUE;
   opentimers_setPeriod(timerId_ushortid_timeout,TIMER_ONESHOT,USHORTIDTIMEOUT);
   opentimers_restart(timerId_ushortid_timeout);
   openqueue_freePacketBuffer(msg);
}

uint16_t ushortid_myid(void) {
    return ushortid_vars.mysid;
}
