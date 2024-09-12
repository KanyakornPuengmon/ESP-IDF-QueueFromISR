#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"
#include "driver/gpio.h"

// 2.2 Define
#define LED_PIN 27
#define PUSH_BUTTON_PIN 33

// 2.3 Create handle
TaskHandle_t myTaskHandle = NULL;
QueueHandle_t queue;

// 2.4 Code ของ Task
void Task(void *arg)
{
    // 1. เตรียมพื้นที่รับข้อมูลผ่าน queue
    char rxBuffer;
    
    // 2. วนลูปรอ
    while (1)
    {
        // 3. ถ้ามีการส่งข้อมูลผ่าน queue handle ที่รอรับ ให้เอาข้อมูลไปแสดงผล
        // queue consumer จะใช้ฟังก์ชัน xQueueReceive
        if (xQueueReceive(queue, &rxBuffer, (TickType_t) 5))
        {
            printf("Button pressed!\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

// 2.5 Code สำหรับบริการอินเตอร์รัพต์จากการกดปุ่ม
void IRAM_ATTR button_isr_handler(void *arg)
{
    // code จาก https://www.freertos.org/a00119.html
    char cIn = '1';
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(queue, &cIn, &xHigherPriorityTaskWoken);

    // Force a context switch if xHigherPriorityTaskWoken is set.
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// 2.6 Code ของ app_main()
void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(PUSH_BUTTON_PIN);
    esp_rom_gpio_pad_select_gpio(LED_PIN);

    gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PUSH_BUTTON_PIN, button_isr_handler, NULL);

    // สร้าง queue ที่บรรจุตัวแปร char จำนวน 1 ตัว
    queue = xQueueCreate(1, sizeof(char));

    // ทดสอบว่าสร้าง queue สำเร็จ?
    if (queue == NULL)
    {
        printf("Failed to create queue\n");
        return;
    }

    // สร้าง Task
    xTaskCreatePinnedToCore(Task, "My_Task", 4096, NULL, 10, &myTaskHandle, 1);
}
