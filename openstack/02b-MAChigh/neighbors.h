#ifndef __NEIGHBORS_H
#define __NEIGHBORS_H

/**
\addtogroup MAChigh
\{
\addtogroup Neighbors
\{
*/
#include "opendefs.h"
#include "icmpv6rpl.h"

//=========================== define ==========================================

#define RETRIAL_STATISTICS        1   // for ublizzard

#define MAXNUMNEIGHBORS           10
#define MAXPREFERENCE             2
#define BADNEIGHBORMAXRSSI        -80 //dBm
#define GOODNEIGHBORMINRSSI       -90 //dBm
#define SWITCHSTABILITYTHRESHOLD  3
#define DEFAULTLINKCOST           15

#define MAXDAGRANK                0xffff
#define DEFAULTDAGRANK            MAXDAGRANK
#define MINHOPRANKINCREASE        256  //default value in RPL and Minimal 6TiSCH draft


//=========================== typedef =========================================

// Compatiable defination in opendef.h
/*BEGIN_PACK
typedef struct {
   bool             used;
   uint8_t          parentPreference;
   bool             stableNeighbor;
   uint8_t          switchStabilityCounter;
   open_addr_t      addr_64b;
   dagrank_t        DAGrank;
   int8_t           rssi;
   uint8_t          numRx;
   uint8_t          numTx;
   uint8_t          numTxACK;
   uint8_t          numWraps;//number of times the tx counter wraps. can be removed if memory is a restriction. also check openvisualizer then.
   asn_t            asn;
   uint8_t          joinPrio;
   bool             f6PNORES;
} neighborRow_t;
END_PACK*/

BEGIN_PACK
typedef struct {
   uint8_t         row;
   neighborRow_t   neighborEntry;
} debugNeighborEntry_t;
END_PACK

BEGIN_PACK
typedef struct {
   uint8_t         last_addr_byte;   // last byte of the neighbor's address
   int8_t          rssi;
   uint8_t         parentPreference;
   dagrank_t       DAGrank;
   uint16_t        asn; 
} netDebugNeigborEntry_t;
END_PACK

//=========================== module variables ================================
   
typedef struct {
   neighborRow_t        neighbors[MAXNUMNEIGHBORS];
   dagrank_t            myDAGrank;
   uint8_t              debugRow;
   icmpv6rpl_dio_ht*    dio; //keep it global to be able to debug correctly.
#ifdef RETRIAL_STATISTICS
   uint32_t             numTxTotal;
   uint32_t             numTxAckTotal;
#endif
} neighbors_vars_t;

typedef struct {
   bool               usedPrimary;
   uint8_t            addrPrimary[8];
   bool               usedBackup;
   uint8_t            addrBackup[8];
} addrParents_vars_t;

//=========================== prototypes ======================================

void          neighbors_init(void);

// getters
dagrank_t     neighbors_getNeighborRank(uint8_t index);
dagrank_t     neighbors_getMyDAGrank(void);
uint8_t       neighbors_getNumNeighbors(void);
uint16_t      neighbors_getLinkMetric(uint8_t index);
bool          neighbors_getPreferredParentEui64(open_addr_t* addressToWrite);
open_addr_t*  neighbors_getKANeighbor(uint16_t kaPeriod);
bool          neighbors_getNeighborNoResource(uint8_t index);
// setters
void          neighbors_setNeighborRank(uint8_t index, dagrank_t rank);
void          neighbors_setNeighborNoResource(open_addr_t* address);
void          neighbors_setPreferredParent(uint8_t index, bool isPreferred);

// interrogators
bool          neighbors_isStableNeighbor(open_addr_t* address);
bool          neighbors_isStableNeighborByIndex(uint8_t index);
bool          neighbors_isPreferredParent(open_addr_t* address);
bool          neighbors_isNeighborWithLowerDAGrank(uint8_t index);
bool          neighbors_isNeighborWithHigherDAGrank(uint8_t index);

// updating neighbor information
void          neighbors_indicateRx(
   open_addr_t*         l2_src,
   int8_t               rssi,
   asn_t*               asnTimestamp,
   bool                 joinPrioPresent,
   uint8_t              joinPrio
);
void          neighbors_indicateTx(
   open_addr_t*         dest,
   uint8_t              numTxAttempts,
   bool                 was_finally_acked,
   asn_t*               asnTimestamp
);
void          neighbors_indicateRxDIO(OpenQueueEntry_t* msg);


// get addresses
bool          neighbors_getNeighborEui64(open_addr_t* address,uint8_t addr_type,uint8_t index);
bool          neighbors_getNeighbor(open_addr_t* address,uint8_t addr_type,uint8_t index);
// maintenance
void          neighbors_removeOld(void);
// debug
bool          debugPrint_neighbors(void);

void          neighbors_get_retrial_statistics(uint32_t* num1, uint32_t* num2);

void          neighbors_get3parents(uint8_t* ptr);
void          neighbors_set2parents(uint8_t* ptr, uint8_t num);
bool          neighbors_isDestRoot_Primary(open_addr_t* address);
bool          neighbors_getPrimary(open_addr_t* addressToWrite);
bool          neighbors_getBackup(open_addr_t* addressToWrite);

static const uint8_t ipAddr_Root[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                           0x00, 0x12, 0x4b, 0x00, 0x06, 0x0d, 0x86, 0x99};

/**
\}
\}
*/

#endif
