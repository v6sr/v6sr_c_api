add_library( PingCommon ping_common.c )
set_property (TARGET PingCommon PROPERTY C_STANDARD 99)
add_executable ( SRHPingServer srh_ping_server.c )
set_property (TARGET SRHPingServer PROPERTY C_STANDARD 99)

target_link_libraries( PingCommon SegmentRoutingAPI )
target_link_libraries( SRHPingServer PingCommon SegFault )
