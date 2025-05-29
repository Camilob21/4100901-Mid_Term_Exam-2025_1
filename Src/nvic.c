/**
 ******************************************************************************
 * @file           : nvic.c
 * @author         : Sam C
 * @brief          : NVIC driver for STM32L476RGTx
 ******************************************************************************
 */
#include "nvic.h"
#include "rcc.h"  // Para habilitar relojes de GPIOC y SYSCFG
#include "uart.h"


#define NVIC_ENABLE_IRQ(IRQn) (NVIC->ISER[IRQn / 32U] |= (1UL << (IRQn % 32U)))


void nvic_exti_pc13_button_enable(void)  
{
    // 1. Habilitar el reloj para SYSCFG
    rcc_syscfg_clock_enable(); // SYSCFG es necesario para mapear EXTI a GPIO

    // 2. Configurar la línea EXTI13 (SYSCFG_EXTICR)
    SYSCFG->EXTICR[3] &= ~(0x000FU << 4);  // Limpiar campo EXTI13 (bits 7-4)
    SYSCFG->EXTICR[3] |=  (0x0002U << 4);  // Conectar EXTI13 a PC13 (0b0010 para PCx)

    // 3. Configurar la línea EXTI13 para interrupción (EXTI_IMR1)
    EXTI->IMR1 |= (1U << 13);

    // 5. Configurar el trigger de flanco de bajada (EXTI_FTSR1)
    EXTI->FTSR1 |= (1U << 13); // Habilitar trigger de flanco de bajada para línea 13
    EXTI->RTSR1 &= ~(1U << 13); // Deshabilitar (no necesitamos detectar el flanco de subida)

    // 6. Habilitar la interrupción EXTI15_10 en el NVIC
    NVIC_ENABLE_IRQ(EXTI15_10_IRQn);
}

void nvic_usart2_irq_enable(void)
{
    // Habilitar interrupción de recepción (RXNEIE - Read Data Register Not Empty Interrupt Enable)
    // Esto hará que se genere una interrupción cuando RDR tenga un dato.
    USART2->CR1 |= 0x01 << 5;
    NVIC_ENABLE_IRQ(USART2_IRQn);
}
