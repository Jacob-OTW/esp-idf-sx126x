#include "Arduino.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ra01s.h"

static const char* TAG = "LoRa";

void task_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1261/62/68 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		int txLen = sprintf((char *)buf, "Hello World %"PRIu32, nowTick);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", txLen);

		// Wait for transmission to complete
		if (LoRaSend(buf, txLen, SX126x_TXMODE_SYNC) == false) {
			ESP_LOGE(pcTaskGetName(NULL),"LoRaSend fail");
		}

		// Do not wait for the transmission to be completed
		//LoRaSend(buf, txLen, SX126x_TXMODE_ASYNC );

		int lost = GetPacketLost();
		if (lost != 0) {
			ESP_LOGW(pcTaskGetName(NULL), "%d packets lost", lost);
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	} // end while
}

void task_rx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1261/62/68 is 255
	while(1) {
		uint8_t rxLen = LoRaReceive(buf, sizeof(buf));
		if ( rxLen > 0 ) { 
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);

			int8_t rssi, snr;
			GetPacketStatus(&rssi, &snr);
			ESP_LOGI(pcTaskGetName(NULL), "rssi=%d[dBm] snr=%d[dB]", rssi, snr);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	} // end while
}

extern "C" void app_main()
{
  initArduino();

  uint32_t frequencyInHz = 868000000;
  ESP_LOGI(TAG, "Frequency is %lu HZ", frequencyInHz);

  LoRaInit();

  int8_t txPowerInDbm = 22;
  float tcxoVoltage = 3.3;
  bool useRegulatorLDO = true;

  if (LoRaBegin(frequencyInHz, txPowerInDbm, tcxoVoltage, useRegulatorLDO) != 0) {
    ESP_LOGE(TAG, "Does not recognize the module");
    while(1) {
      vTaskDelay(1);
    }
  }

  uint8_t spreadingFactor = 7;
	uint8_t bandwidth = 4;
	uint8_t codingRate = 1;
	uint16_t preambleLength = 8;
	uint8_t payloadLen = 0;
	bool crcOn = true;
	bool invertIrq = false;

  LoRaConfig(spreadingFactor, bandwidth, codingRate, preambleLength, payloadLen, crcOn, invertIrq);

  xTaskCreate(&task_tx, "TX", 1024*4, NULL, 5, NULL);

  //xTaskCreate(&task_rx, "RX", 1024*4, NULL, 5, NULL);
}