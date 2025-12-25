# GAP (Goggles Audio Player) Header Files
#
# This file lists all header files for the GAP library
# Included from gap/CMakeLists.txt

# Private headers (internal implementation)
set(HEADERS
    ap_buffer.h
    ap_config.h
    ap_connect.h
    ap_convert.h
    ap_crossfader.h
    ap_decoder_plugin.h
    ap_decoder_thread.h
    ap_defs.h
    ap_engine.h
    ap_event_private.h
    ap_format.h
    ap_input_plugin.h
    ap_input_thread.h
    ap_output_plugin.h
    ap_output_thread.h
    ap_packet.h
    ap_reactor.h
    ap_reader_plugin.h
    ap_signal.h
    ap_socket.h
    ap_thread.h
    ap_thread_queue.h
    ap_utils.h
)

# Public API headers (installed for users)
set(PUBLIC_HEADERS
    include/ap.h
    include/ap_app_queue.h
    include/ap_buffer_base.h
    include/ap_buffer_io.h
    include/ap_common.h
    include/ap_device.h
    include/ap_event.h
    include/ap_event_queue.h
    include/ap_http.h
    include/ap_http_client.h
    include/ap_http_response.h
    include/ap_player.h
    include/ap_xml_parser.h
)
