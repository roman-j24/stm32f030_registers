// Created thu 07 mar 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && rm *.o *.bin *.elf

#include <stdint.h>

int main(void)
{
	(*(volatile uint32_t *)(0x40021000U + 0x14)) |= (1 << 17);  // RCC_AHBENR, GPIOAEN: I/O port A clock enable
	(*(volatile uint32_t *)(0x40021000U + 0x1C)) |= (1 << 1);   // RCC_APB1ENR, TIM3EN: TIM3 timer clock enable

	// Configure PA7 as output mode
	(*(volatile uint32_t *)(0x48000000U + 0x00)) &= ~(3 << 14); // Clear mode bits
	(*(volatile uint32_t *)(0x48000000U + 0x00)) |= (1 << 14);  // Set output mode

	// Configure TIM3
	(*(volatile uint32_t *)(0x40000400U + 0x00)) &= ~(1 << 0);  // Disable TIM3
	// TIM3_PSC, PSC[15:0]: Prescaler value (SystemCoreClock/TIM3_PSC - 1)
	(*(volatile uint32_t *)(0x40000400U + 0x28)) = 8000 - 1;
	// TIM3_ARR, ARR[15:0]: Low Auto-reload value
	(*(volatile uint32_t *)(0x40000400U + 0x2C)) = 500 - 1;

	while (1)
	{
		(*(volatile uint32_t *)(0x40000400U + 0x00)) |= (1 << 0);      // TIM3_CR1, CEN: Counter enable

		// Check if the update event flag is set
		if ((*(volatile uint32_t *)(0x40000400U + 0x10)) & (1 << 0))   // TIM3_SR, UIF: Update interrupt flag
		{
			// Clear the update event flag
			(*(volatile uint32_t *)(0x40000400U + 0x10)) &= ~(1 << 0); // TIM3_SR, UIF

			// Toggle PA7 pin
			(*(volatile uint32_t *)(0x48000000U + 0x14)) ^= (1 << 7);  // ODR
		}
	}
}
