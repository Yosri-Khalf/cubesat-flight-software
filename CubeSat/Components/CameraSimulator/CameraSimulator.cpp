#include "CubeSat/Components/CameraSimulator/CameraSimulator.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace CubeSat {

CameraSimulator::CameraSimulator(const char* const compName)
    : CameraSimulatorComponentBase(compName) {
    const char* enabled = std::getenv("CUBESAT_USE_ESP32");
    this->m_useEsp32 = enabled != nullptr && std::strcmp(enabled, "1") == 0;
}

CameraSimulator::~CameraSimulator() {
    if (this->m_esp32Socket >= 0) {
        ::close(this->m_esp32Socket);
    }
}

bool CameraSimulator::pollEsp32(U64 nowUs) {
    if (nowUs >= this->m_nextSubscribeUs) {
        this->m_nextSubscribeUs = nowUs + 1000000;

        if (this->m_esp32Socket < 0) {
            const char* ip = std::getenv("CUBESAT_ESP32_IP");
            if (ip == nullptr || *ip == '\0') {
                ip = "192.168.4.1";
            }

            sockaddr_in board{};
            board.sin_family = AF_INET;
            board.sin_port = htons(4210);

            if (::inet_pton(AF_INET, ip, &board.sin_addr) != 1) {
                return false;
            }

            const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (fd >= 0) {
                if (::connect(fd, reinterpret_cast<const sockaddr*>(&board), sizeof(board)) == 0) {
                    this->m_esp32Socket = fd;
                } else {
                    ::close(fd);
                }
            }
        }

        if (this->m_esp32Socket >= 0) {
            ::send(this->m_esp32Socket, "H", 1, MSG_DONTWAIT);
        }
    }

    if (this->m_esp32Socket < 0) {
        return false;
    }

    bool receivedHeartbeat = false;
    for (unsigned int i = 0; i < 16; ++i) {
        char packet[16];
        const auto count = ::recv(this->m_esp32Socket, packet, sizeof(packet), MSG_DONTWAIT);
        if (count < 0) {
            break;
        }

        if (count > 0 && count <= 10 &&
            std::all_of(packet, packet + count, [](char digit) {
                return digit >= '0' && digit <= '9';
            })) {
            receivedHeartbeat = true;
        }
    }
    return receivedHeartbeat;
}

void CameraSimulator::run_handler(FwIndexType, U32) {
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    const U64 nowUs = static_cast<U64>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    const bool newHeartbeat = !this->m_useEsp32 || this->pollEsp32(nowUs);

    if (this->m_responding && newHeartbeat) {
        this->m_lastEsp32BeatUs = nowUs;
        if (this->isConnected_heartbeatOut_OutputPort(0)) {
            ++this->m_heartbeatsSent;
            this->heartbeatOut_out(0, this->m_heartbeatsSent, nowUs);
        }
    }

    const bool linkFresh = !this->m_useEsp32 ||
                           (this->m_lastEsp32BeatUs != 0 &&
                            nowUs - this->m_lastEsp32BeatUs < 500000);
    this->tlmWrite_Responding(this->m_responding && linkFresh);
    this->tlmWrite_HeartbeatsSent(this->m_heartbeatsSent);
}

void CameraSimulator::FAIL_CAMERA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->m_responding) {
        this->m_responding = false;
        this->m_lastEsp32BeatUs = 0;
        this->log_ACTIVITY_HI_CameraSilenced();
    }

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void CameraSimulator::RESTORE_CAMERA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->m_responding) {
        this->m_responding = true;
        this->log_ACTIVITY_HI_CameraRestored();
    }

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace CubeSat
