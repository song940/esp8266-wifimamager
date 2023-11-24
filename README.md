# Wi-Fi Manager

Wifi Manager for ESP8266 and ESP32.

```patch
[env:esp01]
platform = espressif8266
board = esp12e
framework = arduino

lib_deps = 
+  https://github.com/song940/esp8266-wifimamager
```

```cpp
#include <WiFiManager.h>

WiFiManager wifiManager("ESP-AP");

// Add custom parameters for configuring MQTT server information
WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT Server", "192.168.8.160", 40);
WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", "1883", 6);
WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", "mqtt", 40);
WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", "mqtt123", 40);

void setup() {
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.autoConnect();
}

void loop() {
    // loop
}
```

## License

This project is licensed under the MIT license.