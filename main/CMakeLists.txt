idf_component_register(SRCS "smart_light.cpp" "LedRing.cpp" SmartLightFSM.cpp MockLightState.cpp ble/BleSetupState.cpp mqtt/MqttSlaveState.cpp tcp/TcpSlaveState.cpp wifi/WiFi.cpp
                    INCLUDE_DIRS "." "../components/arduino-esp32-2.0.1/libraries")
