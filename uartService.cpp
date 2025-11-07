#include <string.h>
#include "esp_log.h"
#include <esp_err.h>
#include "driver/uart.h"
#include "uartservice.h"

UartService::UartService(UartServiceConfig _config) : config(_config), onReceive(nullptr)
{
}

UartService::~UartService()
{
  ESP_LOGD(TAGUART, "Destroying instance");
  uart_driver_delete(config.uartPort);
}

void UartService::init()
{
  ESP_LOGI(TAGUART, "Initializing service ...");

  // Set up UART
  ESP_ERROR_CHECK(uart_param_config(config.uartPort, &config.uartConfig));
  ESP_ERROR_CHECK(uart_set_pin(config.uartPort, config.gpioTx, config.gpioRx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(config.uartPort, 512, 512, 20, &uartQueue, 0));
  xTaskCreate(onEvent,
              "uart_event_task", 4096, this, 10, NULL);
}

int UartService::sendString(std::string text)
{
  ESP_LOGD(TAGUART, "Sending text: %s", text.c_str());
  return uart_write_bytes(config.uartPort, text.c_str(), text.length());
  return 0;
}

void UartService::onEvent(void *pvParameters)
{
  UartService *instance = static_cast<UartService *>(pvParameters);

  uart_event_t event;
  uint8_t data[1024];

  while (1)
  {

    // Wait for UART event
    if (xQueueReceive(instance->uartQueue, &event, portMAX_DELAY))
    {
      switch (event.type)
      {
      case UART_DATA:
      {
        // Data received, read it
        int length = uart_read_bytes(instance->config.uartPort, data, event.size, pdMS_TO_TICKS(100));
        if (length > 0)
        {
          ESP_LOGD(TAGUART, "Received size %d", length);
          if (instance->onReceive)
          {
            std::vector buffer(data, data + length);
            ESP_LOGD(TAGUART, "Calling onReceive handler");
            instance->onReceive(buffer);
          }
        }
        break;
      }

      case UART_FIFO_OVF:
        ESP_LOGW(TAGUART, "Hardware FIFO Overflow");
        uart_flush_input(instance->config.uartPort);
        xQueueReset((QueueHandle_t)pvParameters);
        break;

      case UART_BUFFER_FULL:
        ESP_LOGW(TAGUART, "Ring Buffer Full");
        uart_flush_input(instance->config.uartPort);
        xQueueReset((QueueHandle_t)pvParameters);
        break;

      case UART_PARITY_ERR:
        ESP_LOGW(TAGUART, "Parity Error");
        break;

      case UART_FRAME_ERR:
        ESP_LOGW(TAGUART, "Frame Error");
        break;

      default:
        ESP_LOGW(TAGUART, "Unhandled UART event: %d", event.type);
        break;
      }
    }
  }
}
