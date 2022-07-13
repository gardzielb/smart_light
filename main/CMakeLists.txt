idf_component_register(SRCS "smart_light.cpp" "light/LedRing.cpp" SmartLightFSM.cpp light/BlinkState.cpp setup/BleSetupState.cpp mqtt/MqttSlaveState.cpp tcp/TcpSlaveState.cpp wifi/WiFi.cpp light/LightController.cpp setup/SetupStorage.cpp
                    INCLUDE_DIRS "." "../components/arduino-esp32-2.0.1/libraries")
