#ifndef CubeSat_PayloadManager_HPP
#define CubeSat_PayloadManager_HPP

#include "CubeSat/Components/PayloadManager/PayloadManagerComponentAc.hpp"

namespace CubeSat {

class PayloadManager final : public PayloadManagerComponentBase {
  public:
    explicit PayloadManager(const char* const compName);
    ~PayloadManager() override;

  private:
    enum class Mode { Standby, Imaging, Safe };
    enum class OrbitCaptureState : U32 {
        Idle = 0,
        Armed = 1,
        Capturing = 2,
        Complete = 3,
        Aborted = 4,
        Skipped = 5,
    };

    Mode m_mode = Mode::Standby;
    U32 m_captureLimit = 1;
    U32 m_framesCaptured = 0;
    U64 m_watchStartUs = 0;
    U64 m_lastHeartbeatUs = 0;
    U32 m_lastHeartbeatSequence = 0;
    U32 m_recoveryHeartbeats = 0;
    U64 m_orbitEpochUs = 0;
    U64 m_orbitPassStartUs = 0;
    U32 m_orbitCaptureStartPhase = 0;
    U32 m_orbitCaptureStopPhase = 0;
    OrbitCaptureState m_orbitCaptureState = OrbitCaptureState::Idle;

    void RECOVER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void ARM_ORBIT_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void publishTelemetry();
    void checkCameraTimeout(U64 nowUs);
    U32 orbitPhase(U64 nowUs) const;
    void updateOrbit(U64 nowUs);
    void stopOrbitCapture(U64 nowUs);
    void completeOrbitCapture(U64 nowUs);
    void skipOrbitCapture();
    bool imagingReady() const;
    void beginImaging();
    U32 m_batteryPercent = 100;

    void enterSafeMode();
    void handleCameraTimeout(U64 silenceUs);
    void batteryPercentIn_handler(FwIndexType portNum, U32 percent) override;

    void run_handler(FwIndexType portNum, U32 context) override;
    void checkCamera_handler(FwIndexType portNum, U32 context) override;
    void cameraHeartbeatIn_handler(FwIndexType portNum, U32 sequence, U64 sentAtUs) override;

    void SET_CAPTURE_LIMIT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 limit) override;
    void START_IMAGING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void STOP_IMAGING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
};

}  // namespace CubeSat

#endif
