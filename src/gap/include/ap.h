#ifndef AP_H
#define AP_H

#if __GNUC__ >= 4
  #define GMAPI __attribute__ ((visibility("default")))
#else
  #define GMAPI
#endif

#include <ap_version.h>
#include <ap_event.h>
#include <ap_event_queue.h>
#include <ap_app_queue.h>
#include <ap_player.h>

using namespace ap;

#endif

