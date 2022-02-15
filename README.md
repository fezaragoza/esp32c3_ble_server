# esp32c3_ble_server
BLE Server using GAP and GATT ESP APIs

| Supported Targets | ESP32 | ESP32-C3 |
| ----------------- | ----- | -------- |

ESP-IDF Gatt Server Demo
========================

This project uses the ESP APIs to create a GATT service by adding attributes one by one.

Though, it is possible to create a GATT service with an attribute table, which releases the user from adding attributes one by one. For more information about this method, please refer to [gatt_server_service_table_demo](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/ble/gatt_server_service_table).

The code creates a GATT service and then starts advertising, waiting to be connected to a GATT client. 

To test it, you can run the [ble_client](https://github.com/fezaragoza/esp32c3_ble_client) code, which can scan for and connect to this server automatically. They will start exchanging data once the GATT client has enabled the notification function of the GATT server.

Please check the [docs](docs/Gatt_Server_Example_Walkthrough.md) for more information about this.

