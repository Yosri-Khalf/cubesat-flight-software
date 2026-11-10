module CubeSat {

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 64 * 1024
  }

  instance payloadManager: PayloadManager base id 0x10005000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 50

  instance rateGroup_1Hz: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  instance rateGroup_0_5Hz: Svc.ActiveRateGroup base id 0x10002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  instance rateGroup_0_25Hz: Svc.ActiveRateGroup base id 0x10003000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 41

  instance cmdSeq: Svc.CmdSequencer base id 0x10004000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 40

  instance chronoTime: Svc.ChronoTime base id 0x10010000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x10011000

  instance systemResources: Svc.SystemResources base id 0x10012000

  instance timer: Svc.LinuxTimer base id 0x10013000

  instance comDriver: Drv.TcpClient base id 0x10014000

  instance heartbeatDriver: Svc.RateGroupDriver base id 0x10015000

  instance batterySimulator: BatterySimulator base id 0x10008000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 48

  instance cameraSimulator: CameraSimulator base id 0x10007000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 49

  instance rateGroup_10Hz: Svc.ActiveRateGroup base id 0x10006000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 44

}
