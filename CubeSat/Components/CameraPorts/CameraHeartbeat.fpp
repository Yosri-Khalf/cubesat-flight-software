module CubeSat {
    @ A heartbeat carries a sequence number and a monotonic timestamp in microseconds
    port CameraHeartbeat(sequence: U32, sentAtUs: U64)
}
