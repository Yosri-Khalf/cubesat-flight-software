module CubeSat {
    @ Controls simulated image capture
    active component PayloadManager {

        @ Set the number of images to capture
        async command SET_CAPTURE_LIMIT(
            limit: U32
        ) opcode 0

        @ Begin a new imaging run
        async command START_IMAGING opcode 1

        @ Stop the current imaging run
        async command STOP_IMAGING opcode 2

        @ Receive periodic timer ticks
        async input port run: Svc.Sched

        @ Configured image capture limit
        telemetry CaptureLimit: U32

        @ Images captured during the current run
        telemetry FramesCaptured: U32

        @ Whether imaging is currently active
        telemetry ImagingActive: bool

        async input port cameraHeartbeatIn: CameraHeartbeat
        telemetry LastCameraHeartbeat: U32

        async input port checkCamera: Svc.Sched
        telemetry SafeMode: bool
        telemetry RecoveryHeartbeats: U32
        async command RECOVER opcode 3
        @ Schedule imaging for the next full simulated target pass
        async command ARM_ORBIT_CAPTURE opcode 4
        event RecoveryRejected(beats: U32) severity warning low format "Recovery rejected: received {} of 3 consecutive camera heartbeats"
        event RecoveryAccepted severity activity high format "Recovery accepted: payload returned to standby"
        event CameraTimeout(silenceMs: U64) severity warning high format "Entered SAFE after {} ms without a camera heartbeat"

        async input port batteryPercentIn: BatteryReading
        telemetry BatteryPercent: U32
        event LowBattery(percent: U32) severity warning high format "Battery at {}%: SAFE required"
        event RecoveryBatteryLow(percent: U32) severity warning low format "Recovery rejected: battery at {}%, need at least 30%"

        @ Simulated orbit position in hundredths of a degree
        telemetry OrbitPhase: U32
        @ Scheduled capture state: 0 idle, 1 armed, 2 capturing, 3 complete, 4 aborted, 5 skipped
        telemetry OrbitCaptureState: U32
        @ Orbit phase when the scheduled capture began, in hundredths of a degree
        telemetry OrbitCaptureStartPhase: U32
        @ Orbit phase when the scheduled capture stopped, in hundredths of a degree
        telemetry OrbitCaptureStopPhase: U32

        event OrbitCaptureArmed severity activity high format "Capture armed for the next full target pass"
        event OrbitCaptureStarted(orbitAngle: U32) severity activity high format "Scheduled capture started at orbit phase {} centidegrees"
        event OrbitCaptureComplete(frames: U32) severity activity high format "Scheduled capture finished with {} frames"
        event OrbitCaptureAborted(orbitAngle: U32) severity warning low format "Scheduled capture aborted at orbit phase {} centidegrees"
        event OrbitCaptureSkipped severity activity high format "Scheduled pass skipped or canceled before imaging"

        time get port timeCaller

        import Fw.Command
        import Fw.Event
        import Fw.Channel
    }
}
