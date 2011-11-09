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
#include <ap_device.h>
#include <ap_player.h>
#include <ap_common.h>
#include <ap_memory_buffer.h>
#include <ap_http.h>

using namespace ap;

#endif

