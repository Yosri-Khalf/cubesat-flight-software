module CubeSat {
    @ Simulates battery charge for fault testing
    active component BatterySimulator {
        @ Set the simulated charge from 0 to 100 percent
        async command SET_BATTERY_PERCENT(percent: U32) opcode 0

        @ Current simulated battery charge
        telemetry BatteryPercent: U32
        output port batteryPercentOut: BatteryReading

        time get port timeCaller
        import Fw.Command
        import Fw.Channel
    }
}
