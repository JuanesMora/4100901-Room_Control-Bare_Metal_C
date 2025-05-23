/**
 ******************************************************************************
 * @file           : room_control.c
 * @author         : Juan Esteban Mora
 * @brief          : Room control driver for STM32L476RGTx
 ******************************************************************************
 */

#include "room_control.h"

#include "gpio.h"
#include "systick.h"
#include "uart.h"
#include "tim.h"

// Variables internas
static uint32_t last_heartbeat_tick = 0;
static uint8_t led_on = 0;
static uint32_t led_timeout = 0;

void room_control_app_init(void)
{
    // Apagar LED ON/OFF y PWM al inicio
    gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
    tim3_ch1_pwm_set_duty_cycle(0);
    uart2_send_string("Sistema iniciado.\r\n");
}

void room_control_app_update(void)
{
    // Heartbeat cada 500 ms
    if (systick_get_tick() - last_heartbeat_tick >= 500)
    {
        gpio_toggle_pin(HEARTBEAT_LED_PORT, HEARTBEAT_LED_PIN);
        last_heartbeat_tick = systick_get_tick();
    }

    // Timeout de 3 segundos para LED externo ON/OFF
    if (led_on && (systick_get_tick() - led_timeout >= 3000))
    {
        gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
        led_on = 0;
        uart2_send_string("LED externo apagado (timeout).\r\n");
    }
}

void room_control_on_button_press(void)
{
    static uint32_t last_press = 0;
    uint32_t now = systick_get_tick();

    if (now - last_press < 200) return; // Anti-rebote
    last_press = now;

    gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_SET);
    led_on = 1;
    led_timeout = now;

    uart2_send_string("Boton B1: Presionado.\r\n");
}

void room_control_on_uart_receive(char received_char)
{
    switch (received_char)
    {
        case 'H':
        case 'h':
            tim3_ch1_pwm_set_duty_cycle(100);
            uart2_send_string("PWM LED: 100%.\r\n");
            break;
        case 'L':
        case 'l':
            tim3_ch1_pwm_set_duty_cycle(0);
            uart2_send_string("PWM LED: 0%.\r\n");
            break;
        case 't':
        case 'T':
        {
            static uint8_t onoff_state = 0;
            onoff_state = !onoff_state;
            gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, onoff_state);
            uart2_send_string(onoff_state ? "LED ON/OFF: Encendido (toggle).\r\n" : "LED ON/OFF: Apagado (toggle).\r\n");
            break;
        }
        default:
            uart2_send_string("Comando desconocido.\r\n");
            break;
    }
}
