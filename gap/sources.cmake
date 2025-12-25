# GAP (Goggles Audio Player) Source Files
#
# This file lists all source files for the GAP library
# Included from gap/CMakeLists.txt

# Core library sources
set(SOURCES
    ap_app_queue.cpp
    ap_buffer.cpp
    ap_buffer_io.cpp
    ap_connect.cpp
    ap_convert.cpp
    ap_crossfader.cpp
    ap_decoder_plugin.cpp
    ap_decoder_thread.cpp
    ap_device.cpp
    ap_engine.cpp
    ap_event.cpp
    ap_format.cpp
    ap_http_client.cpp
    ap_http_response.cpp
    ap_input_plugin.cpp
    ap_input_thread.cpp
    ap_output_thread.cpp
    ap_packet.cpp
    ap_player.cpp
    ap_reactor.cpp
    ap_reader_plugin.cpp
    ap_signal.cpp
    ap_socket.cpp
    ap_thread.cpp
    ap_thread_queue.cpp
    ap_utils.cpp
    ap_xml_parser.cpp
)

# Plugin sources (input/reader plugins)
set(PLUGIN_SOURCES
    plugins/ap_aiff.cpp
    plugins/ap_file.cpp
    plugins/ap_http.cpp
    plugins/ap_m3u.cpp
    plugins/ap_pcm.cpp
    plugins/ap_pls.cpp
    plugins/ap_wav.cpp
    plugins/ap_xspf.cpp
)

# Output plugin sources (audio backends)
set(OUTPUT_SOURCES
    plugins/ap_alsa.cpp
    plugins/ap_jack.cpp
    plugins/ap_oss_plugin.cpp
    plugins/ap_pulse.cpp
    plugins/ap_sndio.cpp
    plugins/ap_wavout.cpp
)
