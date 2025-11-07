#pragma once
#include "driver/uart.h"
#include <string>
#include <memory>
#include <functional>

static constexpr const char *TAGUART = "UART";

typedef struct
{
  uart_port_t uartPort;
  int gpioTx;
  int gpioRx;
  uart_config_t uartConfig;
} UartServiceConfig;

class UartService
{
private:
  UartServiceConfig config;
  QueueHandle_t uartQueue;

  static void onEvent(void *pvParameters);

  using OnReceiveHandler = std::function<void(std::vector<uint8_t> data)>;
  OnReceiveHandler onReceive;

public:
  UartService(UartServiceConfig config);
  ~UartService();

  void init();

  void setReceiveHandler(const OnReceiveHandler &handler)
  {
    onReceive = handler;
  }

  int sendString(std::string text);
};
