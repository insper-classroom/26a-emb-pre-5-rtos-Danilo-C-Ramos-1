#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

#define BTN_EVENT_R 1
#define BTN_EVENT_Y 2

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

void btn_callback(uint gpio, uint32_t events) {
    uint8_t button_event = 0;   

    if ((events & GPIO_IRQ_EDGE_FALL) == 0) {
        return;
    }

    if (gpio == BTN_PIN_R) {
        button_event = BTN_EVENT_R;
    } else if (gpio == BTN_PIN_Y) {
        button_event = BTN_EVENT_Y;
    }

    if (button_event != 0) {
        xQueueSendFromISR(xQueueBtn, &button_event, NULL);
    }
}

void led_1_task(void *p) {
    (void)p;
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int blink_enabled = 0;
    int led_state = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE) {
            blink_enabled = !blink_enabled;
            if (!blink_enabled) {
                led_state = 0;
                gpio_put(LED_PIN_R, 0);
            }
        }

        if (blink_enabled) {
            led_state = !led_state;
            gpio_put(LED_PIN_R, led_state);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void led_2_task(void *p) {      
    (void)p;
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int blink_enabled = 0;
    int led_state = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE) {
            blink_enabled = !blink_enabled;
            if (!blink_enabled) {
                led_state = 0;
                gpio_put(LED_PIN_Y, 0);
            }
        }

        if (blink_enabled) {
            led_state = !led_state;
            gpio_put(LED_PIN_Y, led_state);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_task(void *p) {
    (void)p;
    uint8_t button_event = 0;

    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        if (xQueueReceive(xQueueBtn, &button_event, portMAX_DELAY) == pdTRUE) {
            if (button_event == BTN_EVENT_R) {
                xSemaphoreGive(xSemaphoreLedR);
            } else if (button_event == BTN_EVENT_Y) {
                xSemaphoreGive(xSemaphoreLedY);
            }
        }
    }
}

int main() {
    xQueueBtn = xQueueCreate(8, sizeof(uint8_t));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}