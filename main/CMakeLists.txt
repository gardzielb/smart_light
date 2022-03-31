idf_component_register(SRCS "smart_light.cpp" "light/LedRing.cpp" SmartLightFSM.cpp light/MockLightState.cpp ble/BleSetupState.cpp mqtt/MqttSlaveState.cpp tcp/TcpSlaveState.cpp wifi/WiFi.cpp light/LightController.cpp
                    INCLUDE_DIRS "." "../components/arduino-esp32-2.0.1/libraries")
