module CubeSat {
    @ Simulates a camera heartbeat and camera faults
    active component CameraSimulator {
        @ Receive a tick from the 10 Hz rate group
        async input port run: Svc.Sched

        @ Send a heartbeat to the payload manager
        output port heartbeatOut: CameraHeartbeat

        @ Simulate a camera that stops responding
        async command FAIL_CAMERA opcode 0

        @ Let the camera respond again
        async command RESTORE_CAMERA opcode 1

        telemetry Responding: bool
        telemetry HeartbeatsSent: U32

        event CameraSilenced severity activity high id 0 format "Simulated camera stopped responding"
        event CameraRestored severity activity high id 1 format "Simulated camera resumed responding"

        time get port timeCaller
        import Fw.Command
        import Fw.Event
        import Fw.Channel
    }
}
