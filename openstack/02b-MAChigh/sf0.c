#include "opendefs.h"
#include "sf0.h"
#include "neighbors.h"
#include "sixtop.h"
#include "scheduler.h"
#include "schedule.h"
#include "idmanager.h"
#include "openapps.h"
#include "forwarding.h"
#include "packetfunctions.h"

//=========================== definition =====================================

#define SF0THRESHOLD      1

//=========================== variables =======================================

sf0_vars_t sf0_vars;

//=========================== prototypes ======================================

void sf0_addCell_task(void);
void sf0_removeCell_task(void);
void sf0_bandwidthEstimate_task(void);

//=========================== public ==========================================

void sf0_init(void) {
    memset(&sf0_vars,0,sizeof(sf0_vars_t));
    sf0_vars.numAppPacketsPerSlotFrame = 0;
}

void sf0_notif_addedCell(void) {
   scheduler_push_task(sf0_addCell_task,TASKPRIO_SF0);
}

void sf0_notif_removedCell(void) {
   scheduler_push_task(sf0_removeCell_task,TASKPRIO_SF0);
}

// this function is called once per slotframe. 
void sf0_notifyNewSlotframe(void) {
   scheduler_push_task(sf0_bandwidthEstimate_task,TASKPRIO_SF0);
}

void sf0_setBackoff(uint8_t value){
    sf0_vars.backoff = value;
}

//=========================== private =========================================

void sf0_addCell_task(void) {
   open_addr_t          neighbor;
   bool                 foundNeighbor;
   
   // get preferred parent
   foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
   if (foundNeighbor==FALSE) {
      return;
   }
   
   if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
      // one sixtop transcation is happening, only one instance at one time
      return;
   }
   // call sixtop
   sixtop_request(
      IANA_6TOP_CMD_ADD,
      &neighbor,
      1
   );
}

void sf0_removeCell_task(void) {
   open_addr_t          neighbor;
   bool                 foundNeighbor;
   
   // get preferred parent
   foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
   if (foundNeighbor==FALSE) {
      return;
   }
   
   if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
      // one sixtop transcation is happening, only one instance at one time
      return;
   }
   // call sixtop
   sixtop_request(
      IANA_6TOP_CMD_DELETE,
      &neighbor,
      1
   );
}

void sf0_bandwidthEstimate_task(void){
    open_addr_t    *neighbor;
    //open_addr_t    anyaddr;
    //bool           foundNeighbor;
    //int8_t         bw_outgoing;
    //int8_t         bw_incoming;
    //int8_t         bw_self;
    uint16_t estimatedNeighborBandwidth, actualNeighborBandwidth, effectiveNeighborBandwidth, diff, lastused;
    uint8_t i, parent;
    
    // do not reserve cells if I'm a DAGroot
    //if (idmanager_getIsDAGroot()){
    //    return;
    //}
    
    if (sf0_vars.backoff>0){
        sf0_vars.backoff -= 1;
        neighbors_notif_newSlot();
        return;
    }

    if(icmpv6rpl_getPreferredParentIndex(&parent) == FALSE) parent = MAXNUMNEIGHBORS;
    i = neighbors_getBandwidthRequest(&actualNeighborBandwidth, &effectiveNeighborBandwidth, &estimatedNeighborBandwidth, &lastused);
    if(i < MAXNUMNEIGHBORS) {
        neighbor = neighbors_indexToAddress(i);
        if(estimatedNeighborBandwidth > effectiveNeighborBandwidth + SF0THRESHOLD || ((i == parent || idmanager_getIsDAGroot()) && estimatedNeighborBandwidth > effectiveNeighborBandwidth)) {
            diff = estimatedNeighborBandwidth - effectiveNeighborBandwidth;
            if(schedule_getNumberOfFreeEntries() >= diff) {
                if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
                // one sixtop transcation is happening, only one instance at one time
                    neighbors_notif_newSlot();
                    return;
                }
                sixtop_request(
                    IANA_6TOP_CMD_ADD,
                    neighbor,
                    diff
                );
            }
        }
        else if(estimatedNeighborBandwidth + SF0THRESHOLD < effectiveNeighborBandwidth && effectiveNeighborBandwidth > lastused) {
            diff = effectiveNeighborBandwidth - estimatedNeighborBandwidth;
            if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
                // one sixtop transcation is happening, only one instance at one time
                neighbors_notif_newSlot();
                return;
            }
            sixtop_request(
                IANA_6TOP_CMD_DELETE,
                neighbor,
                diff
            );
        }
    }

    neighbors_notif_newSlot();
    
    // get preferred parent
    //foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
    //if (foundNeighbor==FALSE) {
    //    return;
    //}
    
    // get bandwidth of outgoing, incoming and self.
    // Here we just calculate the estimated bandwidth for 
    // the application sending on dedicate cells(TX or Rx).
    //anyaddr.type = ADDR_ANYCAST;
    //bw_outgoing = schedule_getNumOfSlotsByType(CELLTYPE_TX, &neighbor);
    //bw_incoming = schedule_getNumOfSlotsByType(CELLTYPE_RX, &anyaddr);
    
    // get self required bandwith, you can design your
    // application and assign bw_self accordingly. 
    // for example:
    //    bw_self = application_getBandwdith(app_name);
    // By default, it's set to zero.
    // bw_self = openapps_getBandwidth(COMPONENT_UINJECT);
    //bw_self = sf0_vars.numAppPacketsPerSlotFrame;
    
    // In SF0, scheduledCells = bw_outgoing
    //         requiredCells  = bw_incoming + bw_self
    // when scheduledCells<requiredCells, add one or more cell
    
    /*if (bw_outgoing <= bw_incoming+bw_self){
        if(schedule_getNumberOfFreeEntries() > 0) {
            if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
                // one sixtop transcation is happening, only one instance at one time
                return;
            }
            sixtop_request(
                IANA_6TOP_CMD_ADD,
                &neighbor,
                bw_incoming+bw_self-bw_outgoing + 1
            );
        }
    } else {
        // remove cell(s)
        if ( (bw_incoming+bw_self) < (bw_outgoing-SF0THRESHOLD)) {
            if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
               // one sixtop transcation is happening, only one instance at one time
               return;
            }
            sixtop_request(
                IANA_6TOP_CMD_CLEAR,
                &neighbor,
                1
            );
        } else {
            // nothing to do
        }
    }*/
}

void sf0_appPktPeriod(uint8_t numAppPacketsPerSlotFrame){
    sf0_vars.numAppPacketsPerSlotFrame = numAppPacketsPerSlotFrame;
}
