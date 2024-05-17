#include <Arduino.h>
#include "IndicatorLight.h"
#include "config.h"

// This task does all the heavy lifting for our application
void indicatorLedTask(void *param)
{
    IndicatorLight *indicator_light = static_cast<IndicatorLight *>(param);
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
    while (true)
    {
        // wait for someone to trigger us
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0)
        {
            switch (indicator_light->getState())
            {
                case OFF:
                {
                    digitalWrite(INDICATOR_LED_PIN, LOW);
                    break;
                }
                case ON:
                {
                    digitalWrite(INDICATOR_LED_PIN, HIGH);
                    break;
                }
                case PULSING:
                {
                    // flash slowly
                    while (indicator_light->getState() == PULSING)
                    {
                        digitalWrite(INDICATOR_LED_PIN, HIGH);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        digitalWrite(INDICATOR_LED_PIN, LOW);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                }
                case SUCCEED:
                {
                    // flash quickly
                    for (int i = 0; i < 10; i++)
                    {
                        digitalWrite(INDICATOR_LED_PIN, HIGH);
                        vTaskDelay(50 / portTICK_PERIOD_MS);
                        digitalWrite(INDICATOR_LED_PIN, LOW);
                        vTaskDelay(50 / portTICK_PERIOD_MS);
                    }
                }
                case FAILURE:
                {
                    // flash slowly
                    for (int i = 0; i < 5; i++)
                    {
                        digitalWrite(INDICATOR_LED_PIN, HIGH);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                        digitalWrite(INDICATOR_LED_PIN, LOW);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                    }
                }
            }
        }
    }
}

IndicatorLight::IndicatorLight()
{
    // printf("Indicator Light setting up");
    pinMode(INDICATOR_LED_PIN, OUTPUT);
    // printf("Pinmode set");
    // start off with the light off
    // printf("Turning on...");
    // digitalWrite(INDICATOR_LED_PIN, HIGH);
    // // printf("On");
    // delay(1000);
    // // printf("Turning off...");
    // digitalWrite(INDICATOR_LED_PIN, LOW);
    m_state = OFF;
    // set up the task for controlling the light
    xTaskCreate(indicatorLedTask, "Indicator LED Task", 4096, this, 1, &m_taskHandle);
}

void IndicatorLight::setState(IndicatorState state)
{
    // printf("changing the state of the light");
    m_state = state;
    xTaskNotify(m_taskHandle, 1, eSetBits);
}

IndicatorState IndicatorLight::getState()
{
    return m_state;
}
