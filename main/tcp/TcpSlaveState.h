//
// Created by bartosz on 3/16/22.
//

#ifndef SMART_LIGHT_TCPSLAVESTATE_H
#define SMART_LIGHT_TCPSLAVESTATE_H

#include "SmartLightFSM.h"

#include <asio.hpp>

#include "light/LightController.h"

#define SL_TCP_PORT 13
#define SL_TCP_BUFFER_SIZE 64


class TcpSlaveState : public SmartLightState {
public:
	TcpSlaveState(SmartLightFSM* fsm);

	~TcpSlaveState();

	void begin() override;

	void loop() override;

private:
	asio::io_context m_ioContext;
	asio::ip::tcp::acceptor m_acceptor;
	uint8_t m_buffer[SL_TCP_BUFFER_SIZE];
};


#endif //SMART_LIGHT_TCPSLAVESTATE_H
