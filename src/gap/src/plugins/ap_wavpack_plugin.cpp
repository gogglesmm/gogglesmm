#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

namespace ap {


class WavPackReader : public ReaderPlugin {
public:
  WavPackReader(AudioEngine*);

  /// Init plugin
  FXbool init();

  /// Format type
  FXuchar format() const { return Format::WavPack; }

  /// Process Input
  ReadStatus process(Packet*);

  /// Destructor
  virtual ~WavPackReader();
  };


WavPackReader::WavPackReader(AudioEngine*e) : ReaderPlugin(e) {
  }

FXbool WavPackReader::init() {
  return true;
  }

ReadStatus WavPackReader::process(Packet*) {
  return ReadError;
  }


}

