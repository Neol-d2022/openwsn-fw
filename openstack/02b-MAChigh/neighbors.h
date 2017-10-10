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

#define MAXPREFERENCE             2
#define BADNEIGHBORMAXRSSI        -80 //dBm
#define GOODNEIGHBORMINRSSI       -90 //dBm
#define SWITCHSTABILITYTHRESHOLD  3
#define DEFAULTLINKCOST           4
#define LARGESTLINKCOST           8

#define MAXDAGRANK                0xffff
#define DEFAULTDAGRANK            MAXDAGRANK
#define MINHOPRANKINCREASE        256  //default value in RPL and Minimal 6TiSCH draft


//=========================== typedef =========================================

BEGIN_PACK
typedef struct {
   uint8_t         row;
   neighborRow_t   neighborEntry;
   uint16_t        bw_used;
   uint16_t        sf_passed;
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
} neighbors_vars_t;

typedef struct {
   uint16_t             neighborsId[MAXNUMNEIGHBORS];
} neighbors_shortid_vars_t;

typedef struct {
   open_addr_t        addrPrimary;
   open_addr_t        addrBackup;
   uint8_t            indexPrimary;
   uint8_t            indexBackup;
   uint8_t            lastReportIndex;
   bool               usedPrimary;
   bool               usedBackup;
} addrParents_vars_t;

typedef struct {
   uint16_t bw_used[MAXNUMNEIGHBORS];
   uint16_t sf_passed[MAXNUMNEIGHBORS];
   uint16_t bw_current[MAXNUMNEIGHBORS];
   uint16_t bw_peak[MAXNUMNEIGHBORS];
   uint16_t tx_period[MAXNUMNEIGHBORS];
   asn_t    lastTX[MAXNUMNEIGHBORS];
} neighbor_bw_vars_t;

//=========================== prototypes ======================================

void          neighbors_init(void);

// getters
dagrank_t     neighbors_getNeighborRank(uint8_t index);
uint8_t       neighbors_getNumNeighbors(void);
uint8_t       neighbors_addressToIndex(open_addr_t* neighbor);
uint8_t       neighbors_nextNull_ushortid(void);
uint16_t      neighbors_get_ushortid(uint8_t neighborIndex);
uint16_t      neighbors_getLinkMetric(uint8_t index);
open_addr_t*  neighbors_getKANeighbor(uint16_t kaPeriod);
open_addr_t*  neighbors_getJoinProxy(void);
bool          neighbors_getNeighborNoResource(uint8_t index);
uint8_t       neighbors_getGeneration(open_addr_t* address);
uint8_t       neighbors_getGenerationByIndex(uint8_t index);
uint8_t       neighbors_getSequenceNumber(open_addr_t* address);
bool          neighbors_getBandwidthStats(uint16_t *used, uint16_t *sfpassed, uint16_t *bwcurrent, uint16_t *bwpeak, uint16_t *txperiod, uint8_t index);
// setters
void          neighbors_setNeighborRank(uint8_t index, dagrank_t rank);
void          neighbors_setNeighborNoResource(open_addr_t* address);
void          neighbors_setPreferredParent(uint8_t index, bool isPreferred);
void          neighbors_set_ushortid(uint8_t neighborIndex, uint16_t _ushortid);
// uhurricane
void          neighbors_get3parents(uint8_t* ptr, uint8_t* numOut);
void          neighbors_set2parents(uint8_t* ptr, uint8_t num);
void          neighbors_getStat(uint8_t index, uint8_t *tx, uint8_t *txAck);
// interrogators
bool          neighbors_isStableNeighbor(open_addr_t* address);
bool          neighbors_isStableNeighborByIndex(uint8_t index);
bool          neighbors_isInsecureNeighbor(open_addr_t* address);
bool          neighbors_isNeighborWithLowerDAGrank(uint8_t index);
bool          neighbors_isNeighborWithHigherDAGrank(uint8_t index);
bool          neighbors_reachedMaxTransmission(uint8_t index);

// updating neighbor information
void          neighbors_indicateRx(
   open_addr_t*         l2_src,
   int8_t               rssi,
   asn_t*               asnTimestamp,
   bool                 joinPrioPresent,
   uint8_t              joinPrio,
   bool                 insecure
);
void          neighbors_indicateTx(
   open_addr_t*         dest,
   uint8_t              numTxAttempts,
   bool                 was_finally_acked,
   asn_t*               asnTimestamp
);
void          neighbors_updateSequenceNumber(open_addr_t* address);
void          neighbors_updateGeneration(open_addr_t* address);
void          neighbors_resetGeneration(open_addr_t* address);
void          neighbors_invaildateGeneration(open_addr_t* address);
bool          neighbors_getInvalidGenerationNeighbor(open_addr_t* address);

// get addresses
bool          neighbors_getNeighborEui64(open_addr_t* address,uint8_t addr_type,uint8_t index);
// maintenance
void          neighbors_removeOld(void);
// debug
bool          debugPrint_neighbors(void);

// from otf
void           neighbors_notifyNewSlotframe(void);
void           neighbors_notifyBandwidthUsed(open_addr_t* address);
uint8_t        neighbors_estimatedBandwidth(uint8_t index);

/**
\}
\}
*/



#endif
