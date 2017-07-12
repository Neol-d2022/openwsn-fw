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
   
}

void uantiloop_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

//=========================== private =========================================
