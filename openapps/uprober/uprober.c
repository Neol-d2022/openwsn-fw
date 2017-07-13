#include "opendefs.h"
#include "uprober.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define UPROBERPERIOD  (2 * SLOTFRAME_LENGTH * 15)

//=========================== variables =======================================

uprober_vars_t uprober_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void uprober_init() {
   // clear local variables
   memset(&uprober_vars,0,sizeof(uprober_vars_t));
   
   uprober_vars.timerId_task = opentimers_create();
   if(uprober_vars.timerId_task == TOO_MANY_TIMERS_ERROR) {
   	printf("[ERROR] %hu Cannot initialize uprober module: TOO_MANY_TIMERS_ERROR\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7]);
   	return;
   }
   opentimers_scheduleIn(uprober_vars.timerId_task, UPROBERPERIOD, TIME_MS, TIMER_PERIODIC, uprober_timer_cb);

   // register at UDP stack
   uprober_vars.desc.port              = WKP_UDP_PROBER;
   uprober_vars.desc.callbackReceive   = &uprober_receive;
   uprober_vars.desc.callbackSendDone  = &uprober_sendDone;
   openudp_register(&uprober_vars.desc);
}

void uprober_receive(OpenQueueEntry_t* request) {
   printf("[INFO] %hu receive prober packet from %hu\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], ((request->l2_nextORpreviousHop).addr_64b)[7]);
   openqueue_freePacketBuffer(request);
}

void uprober_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   uprober_vars.busySending -= 1;
   openqueue_freePacketBuffer(msg);
}

void uprober_timer_cb(opentimers_id_t id) {
   scheduler_push_task(uprober_task_cb,TASKPRIO_COAP);
}

void uprober_task_cb(void) {
   OpenQueueEntry_t* probe;
   open_addr_t neighbor;
   
   if(schedule_getNotUsedTxCell(&neighbor) == 0)
      return;
   
   if(uprober_vars.busySending >= 1)
      return;
   
   probe = openqueue_getFreePacketBuffer(COMPONENT_UPROBER);
   if (probe==NULL) {
      openserial_printError(
         COMPONENT_UPROBER,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   probe->owner                         = COMPONENT_UPROBER;
   probe->creator                       = COMPONENT_UPROBER;
   probe->l4_protocol                   = IANA_UDP;
   probe->l4_destination_port           = WKP_UDP_PROBER;
   probe->l4_sourcePortORicmpv6Type     = WKP_UDP_PROBER;
   probe->l3_destinationAdd.type        = ADDR_128B;
   
   packetfunctions_mac64bToIp128b(idmanager_getMyID(ADDR_PREFIX),&neighbor,&probe->l3_destinationAdd);
   
   printf("[INFO] %hu send probe packet to %hu\n", (idmanager_getMyID(ADDR_64B)->addr_64b)[7], (neighbor.addr_64b)[7]);
   
   if ((openudp_send(probe))==E_FAIL) {
      openqueue_freePacketBuffer(probe);
      return;
   }
   uprober_vars.busySending += 1;
}

//=========================== private =========================================
