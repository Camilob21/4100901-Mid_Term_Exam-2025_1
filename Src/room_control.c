/**
 ******************************************************************************
 * @file           : room_control.c
 * @author         : Sam C
 * @brief          : Room control driver for STM32L476RGTx
 ******************************************************************************
 */
#include "room_control.h" // Para las funciones de control de la sala

#include "gpio.h"    // Para controlar LEDs y leer el botón (aunque el botón es por EXTI)
#include "systick.h" // Para obtener ticks y manejar retardos/tiempos
#include "uart.h"    // Para enviar mensajes
#include "tim.h"     // Para controlar el PWM


static volatile uint32_t g_door_open_tick = 0;
static volatile uint8_t g_door_open = 0;
static volatile uint32_t g_last_button_tick = 0;

static uint8_t lamp_ramp_active = 0;
static uint32_t lamp_ramp_start_tick = 0;
static uint8_t lamp_ramp_level = 0;

void room_control_app_init(void)
{
    gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
    g_door_open = 0;
    g_door_open_tick = 0;

    tim3_ch1_pwm_set_duty_cycle(20); // Lámpara al 20%

    //4. Mensaje de bienvenida
    uart2_send_string("Controlador de sala v1.0\r\n");
    uart2_send_string("Desarrollado: William Camilo Obando.\r\n");
    uart2_send_string("Estado Inicial:\r\n");
    uart2_send_string(" -Lámpara al 20%.\r\n");
    uart2_send_string(" -Puerta cerrada.\r\n");
    
}

void room_control_on_button_press(void)
{
    uint32_t now = systick_get_tick();
    if (now - g_last_button_tick < 50) return;  // Anti-rebote de 50 ms
    g_last_button_tick = now;

    uart2_send_string("Evento: Botón presionado - Abriendo puerta.\r\n");
    
    gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_SET);
    g_door_open_tick = now;
    g_door_open = 1;

    //1. Control automatico de iluminacion: Al presionar debe encender el LED al 100%
    // Luego de 10 segundos, debe volver al brillo anterior configurado por UART.
    tim3_ch1_pwm_set_duty_cycle(100); // Lámpara al 100% al abrir la puerta
    uart2_send_string("Lámpara: brillo al 100%.\r\n");
    systick_delay_ms(10000); // Espera 10 segundos
    if (g_door_open) {
        tim3_ch1_pwm_set_duty_cycle(20); // Vuelve al 20% si la puerta sigue abierta
        uart2_send_string("Lámpara: brillo al 20% tras 10 segundos.\r\n");
    } else {
        tim3_ch1_pwm_set_duty_cycle(70); // Si la puerta se cerró, vuelve al 70%
        uart2_send_string("Lámpara: brillo al 70% tras cerrar la puerta.\r\n");
    }

}

void room_control_on_uart_receive(char cmd)
{
    switch (cmd) {
        case '1':
            tim3_ch1_pwm_set_duty_cycle(100);
            uart2_send_string("Lámpara: brillo al 100%.\r\n");
            break;

        case '2':
        case 's': //  5. Comando UART para ver estado actual
            tim3_ch1_pwm_set_duty_cycle(70);
            uart2_send_string("Lámpara: brillo al 70%.\r\n");
            uart2_send_string("Puerta Abierta.\r\n");
            break;

        case '3':
            tim3_ch1_pwm_set_duty_cycle(50);
            uart2_send_string("Lámpara: brillo al 50%.\r\n");
            break;

        case '4':
            tim3_ch1_pwm_set_duty_cycle(20);
            uart2_send_string("Lámpara: brillo al 20%.\r\n");
            break;

        case '0':
            tim3_ch1_pwm_set_duty_cycle(0);
            uart2_send_string("Lámpara apagada.\r\n");
            break;

        case 'o':
        case 'O':
            gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_SET);
            g_door_open_tick = systick_get_tick();
            g_door_open = 1;
            uart2_send_string("Puerta abierta remotamente.\r\n");
            break;

        case 'c':
        case 'C':
            gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
            g_door_open = 0;
            uart2_send_string("Puerta cerrada remotamente.\r\n");
            break;

         //6. Comando de ayuda UART   
         case '?':
            uart2_send_string("'1'-'4': Ajustar brillo lámpara (100%, 70%, 50%, 20%):\r\n");
            uart2_send_string("'0'   : Apagar lámpara\r\n");
            uart2_send_string("'o'   : Abrir puerta\r\n");
            uart2_send_string("'c'   : Cerrar puerta\r\n");
            uart2_send_string("'s'   : Estado del sistema\r\n");
            uart2_send_string("'?'   : Ayuda\r\n");
            break;

        //7. Transicion gradual del brillo:
        case 'g':
            lamp_ramp_active = 1;
            lamp_ramp_level = 0;
            lamp_ramp_start_tick = systick_get_tick();
            tim3_ch1_pwm_set_duty_cycle(0);
            uart2_send_string("Ramp up: lámpara de 0% a 100%.\r\n");
            break;    

        default:
            uart2_send_string("Comando desconocido.\r\n");
            break;
    }
}

void room_control_tick(void)
{
    if (g_door_open && (systick_get_tick() - g_door_open_tick >= 3000)) {
        gpio_write_pin(EXTERNAL_LED_ONOFF_PORT, EXTERNAL_LED_ONOFF_PIN, GPIO_PIN_RESET);
        uart2_send_string("Puerta cerrada automáticamente tras 3 segundos.\r\n");
        g_door_open = 0;
    }


    static uint8_t lamp_ramp_active = 0;
    static uint32_t lamp_ramp_last_tick = 0;
    static uint8_t lamp_ramp_level = 0;

    if (lamp_ramp_active) {
        if (systick_get_tick() - lamp_ramp_last_tick >= 500) {
            lamp_ramp_last_tick = systick_get_tick();
            tim3_ch1_pwm_set_duty_cycle(lamp_ramp_level * 10);
            lamp_ramp_level++;
            if (lamp_ramp_level > 10) {
                lamp_ramp_active = 0;
                uart2_send_string("Transcision terminada.\r\n");
            }
        }
    }



}
