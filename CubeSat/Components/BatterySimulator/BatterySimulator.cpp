#include "CubeSat/Components/BatterySimulator/BatterySimulator.hpp"

namespace CubeSat {

BatterySimulator::BatterySimulator(const char* const compName)
    : BatterySimulatorComponentBase(compName) {}

BatterySimulator::~BatterySimulator() = default;

void BatterySimulator::SET_BATTERY_PERCENT_cmdHandler(
    FwOpcodeType opCode, U32 cmdSeq, U32 percent
) {
    if (percent > 100) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->m_batteryPercent = percent;
    this->tlmWrite_BatteryPercent(this->m_batteryPercent);
    if (this->isConnected_batteryPercentOut_OutputPort(0)) {
        this->batteryPercentOut_out(0, this->m_batteryPercent);
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace CubeSat
