/**
\brief Applications running on top of the OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, September 2014.
*/

#include "opendefs.h"

// CoAP
#include "c6t.h"
#include "cinfo.h"
#include "cleds.h"
#include "cexample.h"
#include "cstorm.h"
#include "cwellknown.h"
#include "rrt.h"
// TCP
#include "techo.h"
// UDP
#include "uecho.h"
#include "uinject.h"
#include "utyphoon.h"
#include "ublizzard.h"
#include "uhurricane.h"
#include "ushortid.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

//=========================== private =========================================

void openapps_init(void) {
   // CoAP
   c6t_init();
   cinfo_init();
   //cexample_init();
   cleds__init();
   cstorm_init();
   cwellknown_init();
   rrt_init();
   // TCP
   techo_init();
   // UDP
//   uecho_init();
   utyphoon_init();
   //ublizzard_init();
   uhurricane_init();
   ushortid_init();
}
