#ifndef CubeSat_CameraSimulator_HPP
#define CubeSat_CameraSimulator_HPP

#include "CubeSat/Components/CameraSimulator/CameraSimulatorComponentAc.hpp"

namespace CubeSat {

class CameraSimulator final : public CameraSimulatorComponentBase {
  public:
    explicit CameraSimulator(const char* const compName);
    ~CameraSimulator() override;

  private:
    bool m_responding = true;
    U32 m_heartbeatsSent = 0;
    bool m_useEsp32 = false;
    int m_esp32Socket = -1;
    U64 m_nextSubscribeUs = 0;
    U64 m_lastEsp32BeatUs = 0;

    bool pollEsp32(U64 nowUs);

    void run_handler(FwIndexType portNum, U32 context) override;
    void FAIL_CAMERA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void RESTORE_CAMERA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
};

}  // namespace CubeSat

#endif
