#include "CubeSat/Components/PayloadManager/PayloadManager.hpp"

#include <algorithm>
#include <chrono>

namespace CubeSat {

namespace {

constexpr U64 CAMERA_TIMEOUT_US = 500000;
constexpr U64 RECOVERY_MAX_GAP_US = 150000;
constexpr U32 RECOVERY_REQUIRED_BEATS = 3;
constexpr U32 BATTERY_SAFE_PERCENT = 20;
constexpr U32 BATTERY_RECOVERY_PERCENT = 30;

// Accelerated demonstration orbit. The fixed target window runs from 45 to 135 degrees.
constexpr U64 ORBIT_PERIOD_US = 26000000;
constexpr U32 ORBIT_PHASE_PER_TURN = 36000;
constexpr U64 ORBIT_TARGET_ENTRY_US = ORBIT_PERIOD_US / 8;
constexpr U64 ORBIT_TARGET_DURATION_US = ORBIT_PERIOD_US / 4;

U64 monotonicNowUs() {
    using namespace std::chrono;
    const auto elapsed = steady_clock::now().time_since_epoch();
    return static_cast<U64>(duration_cast<microseconds>(elapsed).count());
}

}  // namespace

PayloadManager::PayloadManager(const char* const compName)
    : PayloadManagerComponentBase(compName) {}

PayloadManager::~PayloadManager() = default;

void PayloadManager::publishTelemetry() {
    this->tlmWrite_CaptureLimit(this->m_captureLimit);
    this->tlmWrite_FramesCaptured(this->m_framesCaptured);
    this->tlmWrite_ImagingActive(this->m_mode == Mode::Imaging);
    this->tlmWrite_SafeMode(this->m_mode == Mode::Safe);
    this->tlmWrite_BatteryPercent(this->m_batteryPercent);
    this->tlmWrite_RecoveryHeartbeats(this->m_recoveryHeartbeats);
    this->tlmWrite_OrbitCaptureState(static_cast<U32>(this->m_orbitCaptureState));
    this->tlmWrite_OrbitCaptureStartPhase(this->m_orbitCaptureStartPhase);
    this->tlmWrite_OrbitCaptureStopPhase(this->m_orbitCaptureStopPhase);
}

bool PayloadManager::imagingReady() const {
    return this->m_mode != Mode::Safe && this->m_lastHeartbeatUs != 0 &&
           this->m_batteryPercent > BATTERY_SAFE_PERCENT;
}

void PayloadManager::beginImaging() {
    this->m_framesCaptured = 0;
    this->m_mode = Mode::Imaging;
}

U32 PayloadManager::orbitPhase(U64 nowUs) const {
    const U64 offsetUs = (nowUs - this->m_orbitEpochUs) % ORBIT_PERIOD_US;
    return static_cast<U32>(offsetUs * ORBIT_PHASE_PER_TURN / ORBIT_PERIOD_US);
}

void PayloadManager::skipOrbitCapture() {
    this->m_orbitCaptureState = OrbitCaptureState::Skipped;
    this->log_ACTIVITY_HI_OrbitCaptureSkipped();
}

void PayloadManager::stopOrbitCapture(U64 nowUs) {
    if (this->m_orbitCaptureState == OrbitCaptureState::Armed) {
        this->skipOrbitCapture();
    } else if (this->m_orbitCaptureState == OrbitCaptureState::Capturing) {
        const U64 endUs = this->m_orbitPassStartUs + ORBIT_TARGET_DURATION_US;
        this->m_orbitCaptureStopPhase = this->orbitPhase(std::min(nowUs, endUs));
        this->m_orbitCaptureState = OrbitCaptureState::Aborted;
        this->log_WARNING_LO_OrbitCaptureAborted(this->m_orbitCaptureStopPhase);
    }
}

void PayloadManager::completeOrbitCapture(U64 nowUs) {
    this->m_orbitCaptureStopPhase = this->orbitPhase(nowUs);
    this->m_orbitCaptureState = OrbitCaptureState::Complete;
    this->m_mode = Mode::Standby;
    this->log_ACTIVITY_HI_OrbitCaptureComplete(this->m_framesCaptured);
}

void PayloadManager::updateOrbit(U64 nowUs) {
    if (this->m_orbitEpochUs == 0) {
        this->m_orbitEpochUs = nowUs;
    }
    this->tlmWrite_OrbitPhase(this->orbitPhase(nowUs));

    const U64 endUs = this->m_orbitPassStartUs + ORBIT_TARGET_DURATION_US;
    if (this->m_orbitCaptureState == OrbitCaptureState::Capturing && nowUs >= endUs) {
        this->completeOrbitCapture(endUs);
        this->publishTelemetry();
    } else if (this->m_orbitCaptureState == OrbitCaptureState::Armed && nowUs >= this->m_orbitPassStartUs) {
        if (nowUs < endUs && this->m_mode == Mode::Standby && this->imagingReady()) {
            this->beginImaging();
            this->m_orbitCaptureState = OrbitCaptureState::Capturing;
            this->m_orbitCaptureStartPhase = this->orbitPhase(nowUs);
            this->log_ACTIVITY_HI_OrbitCaptureStarted(this->m_orbitCaptureStartPhase);
        } else {
            this->skipOrbitCapture();
        }
        this->publishTelemetry();
    }
}

void PayloadManager::enterSafeMode() {
    if (this->m_mode == Mode::Safe) {
        return;
    }

    this->m_mode = Mode::Safe;
    this->m_recoveryHeartbeats = 0;
    this->stopOrbitCapture(monotonicNowUs());
    this->publishTelemetry();
}

void PayloadManager::handleCameraTimeout(U64 silenceUs) {
    if (this->m_mode != Mode::Safe) {
        this->enterSafeMode();
        this->log_WARNING_HI_CameraTimeout(silenceUs / 1000);
    }
}

void PayloadManager::batteryPercentIn_handler(FwIndexType, U32 percent) {
    const U32 previousPercent = this->m_batteryPercent;
    this->m_batteryPercent = percent;
    this->tlmWrite_BatteryPercent(percent);

    if (percent <= BATTERY_SAFE_PERCENT) {
        this->enterSafeMode();
        if (previousPercent > BATTERY_SAFE_PERCENT) {
            this->log_WARNING_HI_LowBattery(percent);
        }
    }
}

void PayloadManager::checkCameraTimeout(U64 nowUs) {
    if (this->m_watchStartUs == 0) {
        this->m_watchStartUs = nowUs;
    }

    const U64 referenceUs = this->m_lastHeartbeatUs != 0
                                ? this->m_lastHeartbeatUs
                                : this->m_watchStartUs;
    const U64 silenceUs = nowUs - referenceUs;

    if (this->m_mode == Mode::Safe) {
        if (silenceUs > RECOVERY_MAX_GAP_US && this->m_recoveryHeartbeats != 0) {
            this->m_recoveryHeartbeats = 0;
            this->tlmWrite_RecoveryHeartbeats(0);
        }
        return;
    }

    if (silenceUs >= CAMERA_TIMEOUT_US) {
        this->handleCameraTimeout(silenceUs);
    }
}

void PayloadManager::checkCamera_handler(FwIndexType, U32) {
    const U64 nowUs = monotonicNowUs();
    this->checkCameraTimeout(nowUs);
    this->updateOrbit(nowUs);
}

void PayloadManager::cameraHeartbeatIn_handler(FwIndexType, U32 sequence, U64 sentAtUs) {
    const U64 nowUs = monotonicNowUs();
    if (sentAtUs <= this->m_lastHeartbeatUs || sentAtUs > nowUs ||
        nowUs - sentAtUs >= CAMERA_TIMEOUT_US) {
        return;
    }

    // A heartbeat arriving after a long gap must still trigger SAFE.
    if (this->m_lastHeartbeatUs != 0 &&
        sentAtUs - this->m_lastHeartbeatUs >= CAMERA_TIMEOUT_US) {
        this->handleCameraTimeout(nowUs - this->m_lastHeartbeatUs);
    }

    if (this->m_mode == Mode::Safe) {
        const U32 nextSequence = static_cast<U32>(this->m_lastHeartbeatSequence + 1U);
        const bool consecutive = this->m_recoveryHeartbeats != 0 &&
                                 sequence == nextSequence &&
                                 sentAtUs - this->m_lastHeartbeatUs <= RECOVERY_MAX_GAP_US;

        if (!consecutive) {
            this->m_recoveryHeartbeats = 1;
        } else if (this->m_recoveryHeartbeats < RECOVERY_REQUIRED_BEATS) {
            ++this->m_recoveryHeartbeats;
        }

        this->tlmWrite_RecoveryHeartbeats(this->m_recoveryHeartbeats);
    }

    this->m_lastHeartbeatSequence = sequence;
    this->m_lastHeartbeatUs = sentAtUs;
    this->tlmWrite_LastCameraHeartbeat(sequence);
}

void PayloadManager::run_handler(FwIndexType, U32) {
    const U64 nowUs = monotonicNowUs();
    this->checkCameraTimeout(nowUs);
    this->updateOrbit(nowUs);

    if (this->m_mode == Mode::Imaging) {
        ++this->m_framesCaptured;
        if (this->m_framesCaptured >= this->m_captureLimit) {
            if (this->m_orbitCaptureState == OrbitCaptureState::Capturing) {
                this->completeOrbitCapture(nowUs);
            } else {
                this->m_mode = Mode::Standby;
            }
        }
    }

    this->publishTelemetry();
}

void PayloadManager::SET_CAPTURE_LIMIT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 limit) {
    if (limit < 1 || limit > 20) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->m_mode == Mode::Imaging) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
    }

    this->m_captureLimit = limit;
    this->publishTelemetry();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadManager::START_IMAGING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->checkCameraTimeout(monotonicNowUs());

    if (!this->imagingReady()) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->m_mode == Mode::Imaging) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
    }

    this->beginImaging();
    this->publishTelemetry();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadManager::ARM_ORBIT_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    const U64 nowUs = monotonicNowUs();
    this->checkCameraTimeout(nowUs);
    if (this->m_mode == Mode::Safe) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->m_mode == Mode::Imaging || this->m_orbitCaptureState == OrbitCaptureState::Armed) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
    }

    this->updateOrbit(nowUs);
    const U64 offsetUs = (nowUs - this->m_orbitEpochUs) % ORBIT_PERIOD_US;
    this->m_orbitPassStartUs = nowUs - offsetUs + ORBIT_TARGET_ENTRY_US;
    if (offsetUs >= ORBIT_TARGET_ENTRY_US) {
        this->m_orbitPassStartUs += ORBIT_PERIOD_US;
    }
    this->m_orbitCaptureStartPhase = 0;
    this->m_orbitCaptureStopPhase = 0;
    this->m_orbitCaptureState = OrbitCaptureState::Armed;
    this->publishTelemetry();
    this->log_ACTIVITY_HI_OrbitCaptureArmed();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadManager::RECOVER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->m_mode != Mode::Safe) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->checkCameraTimeout(monotonicNowUs());
    if (this->m_recoveryHeartbeats < RECOVERY_REQUIRED_BEATS) {
        this->log_WARNING_LO_RecoveryRejected(this->m_recoveryHeartbeats);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    if (this->m_batteryPercent < BATTERY_RECOVERY_PERCENT) {
        this->log_WARNING_LO_RecoveryBatteryLow(this->m_batteryPercent);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->m_mode = Mode::Standby;
    this->m_recoveryHeartbeats = 0;
    this->log_ACTIVITY_HI_RecoveryAccepted();
    this->publishTelemetry();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadManager::STOP_IMAGING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->stopOrbitCapture(monotonicNowUs());
    if (this->m_mode == Mode::Imaging) {
        this->m_mode = Mode::Standby;
    }

    this->publishTelemetry();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace CubeSat
