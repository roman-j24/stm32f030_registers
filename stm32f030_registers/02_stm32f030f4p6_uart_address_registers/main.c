// Created thu 29 feb 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000

#include <stdint.h>

// Function to send a single character over UART
void uart_send_char(char c)
{
	// Wait until transmit data register is empty
	while (!(*((volatile uint32_t *)(0x40013800 + 0x1C)) & (1 << 7))); // USART_ISR TXE

	// Send the char via usart
	*((volatile uint32_t *)(0x40013800 + 0x28)) = c; // USART_TDR
}

// Function to send a string over UART
void uart_send_string(const char *str)
{
	while (*str != '\0')
	{
		uart_send_char(*str);
		str++;
	}
}

int main(void)
{
	// Enable GPIOA, GPIOB clock
	*((volatile uint32_t *)(0x40021000 + 0x14)) |= (1 << 17) | (1 << 18); // RCC_AHBENR, set 1 in 17 and 18 registers

	// set PB1 as output
	*((volatile uint32_t *)(0x48000400 + 0x00)) |= (1 << 2); // GPIOx_MODER, 1 is 0b01

	// Enable USART1 clock
	*((volatile uint32_t *)(0x40021000 + 0x18)) |= (1 << 14); // RCC_APB2ENR

	// Configure PA9, PA10 as alternate function mode
	*((volatile uint32_t *)(0x48000000 + 0x00)) |= (2 << 18); // GPIOx_MODER, 2 is 0b10
	*((volatile uint32_t *)(0x48000000 + 0x00)) |= (2 << 20); // GPIOx_MODER

	// no pull up/down PA9, PA10
	*((volatile uint32_t *)(0x48000000 + 0x0C)) |= (0 << 18); // GPIOx_PUPDR, 0 is 0b00
	*((volatile uint32_t *)(0x48000000 + 0x0C)) |= (0 << 20); // GPIOx_PUPDR

	// Configure PA9, PA10 as high-speed mode
	*((volatile uint32_t *)(0x48000000 + 0x08)) |= (3 << 18); // GPIOx_OSPEEDR, 3 is 0b11
	*((volatile uint32_t *)(0x48000000 + 0x08)) |= (3 << 20); // GPIOx_OSPEEDR

	// Configure PA9 tx, PA10 rx as AF1 (USART1)
	*((volatile uint32_t *)(0x48000000 + 0x24)) |= (1 << 4); // GPIOx_AFRH
	*((volatile uint32_t *)(0x48000000 + 0x24)) |= (1 << 8); // GPIOx_AFRH

	// Configure USART1 baud rate to 115200 (8MHz)
	*((volatile uint32_t *)(0x40013800 + 0x0C)) = 0x45; // USART_BRR

	// Enable transmitter and USART1 UE - usart enable, RE - rx enable, TE - tx enable
	*((volatile uint32_t *)(0x40013800 + 0x00)) |= (1 << 0) | (1 << 2) | (1 << 3); // USART_CR1

	uart_send_string("hallo, salut...\n\r");

	*((volatile uint32_t *)(0x48000400 + 0x14)) ^= (1 << 1); // GPIOx_ODR, PB1
	for (uint32_t i = 0; i < 32000 * 5; i++);
	*((volatile uint32_t *)(0x48000400 + 0x14)) ^= (1 << 1); // GPIOx_ODR

	while (1);
}