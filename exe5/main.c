#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueButId;
SemaphoreHandle_t xSemaphore_r;
QueueHandle_t xQueueBut2;
SemaphoreHandle_t xSemaphore_y;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_PIN_R && (events & GPIO_IRQ_EDGE_FALL)) {
        xSemaphoreGiveFromISR(xSemaphore_r, 0);
    }
    if (gpio == BTN_PIN_Y && (events & GPIO_IRQ_EDGE_FALL)) {
        xSemaphoreGiveFromISR(xSemaphore_y, 0);
    }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int led_flag = 0;
    int led_state = 0;

    while (true) {
        if (xQueueReceive(xQueueButId, &led_flag, 0)) {
            led_state = 0;
        }

        if (led_flag) {
            led_state = !led_state;
            gpio_put(LED_PIN_R, led_state);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_1_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);

    int led_flag = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500))) {
            led_flag = !led_flag;
            xQueueOverwrite(xQueueButId, &led_flag);
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int led_flag = 0;
    int led_state = 0;

    while (true) {
        if (xQueueReceive(xQueueBut2, &led_flag, 0)) {
            led_state = 0;
        }

        if (led_flag) {
            led_state = !led_state;
            gpio_put(LED_PIN_Y, led_state);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_2_task(void *p) {
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int led_flag = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphore_y, pdMS_TO_TICKS(500))) {
            led_flag = !led_flag;
            xQueueOverwrite(xQueueBut2, &led_flag);
        }
    }
}

int main() {
    xQueueButId = xQueueCreate(1, sizeof(int));
    xQueueBut2 = xQueueCreate(1, sizeof(int));
    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_1_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);
    xTaskCreate(btn_2_task, "BTN_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}