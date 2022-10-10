//
// Created by bartosz on 3/16/22.
//

#include "TcpSlaveState.h"

#include <esp_log.h>

#define LOG_TAG "TCP_SLAVE"

using tcpip = asio::ip::tcp;

TcpSlaveState::TcpSlaveState(SmartLightFSM* fsm)
	: SmartLightState(fsm), m_acceptor(m_ioContext, tcpip::endpoint(tcpip::v4(), SL_TCP_PORT)) {}

void TcpSlaveState::begin() {
	gpio_set_level(LED_YELLOW_PIN, 1);
	ESP_LOGI(LOG_TAG, "Entering TCP slave state");
}

void TcpSlaveState::loop() {
	tcpip::socket socket(m_ioContext);
	m_acceptor.accept(socket);

	auto bytesRead = socket.read_some(asio::buffer(m_buffer, SL_TCP_BUFFER_SIZE));

	auto& lightController = LightController::get();
	lightController.handleOperation((SmartLightOperation*) m_buffer, m_fsm);
}

TcpSlaveState::~TcpSlaveState() {
	gpio_set_level(LED_YELLOW_PIN, 0);
}
