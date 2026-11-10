#ifndef CubeSat_BatterySimulator_HPP
#define CubeSat_BatterySimulator_HPP

#include "CubeSat/Components/BatterySimulator/BatterySimulatorComponentAc.hpp"

namespace CubeSat {

class BatterySimulator final : public BatterySimulatorComponentBase {
  public:
    explicit BatterySimulator(const char* const compName);
    ~BatterySimulator() override;

  private:
    U32 m_batteryPercent = 100;

    void SET_BATTERY_PERCENT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 percent) override;
};

}  // namespace CubeSat

#endif
