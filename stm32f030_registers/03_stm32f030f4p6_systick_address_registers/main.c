// Created thu 07 mar 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && rm *.o *.bin *.elf

// see p.275 DDI0419C_arm_architecture_v6m_reference_manual.pdf

#include <stdint.h>

// Function to delay for a specific number of milliseconds
void _delay_ms(uint32_t milliseconds)
{
	// Set the reload value for the SysTick timer
	(*(volatile uint32_t *)(0xE000E010U + 0x04)) = (milliseconds * ((8000000 - 1) / 2019)); // SYST_RVR
	// RELOAD - The value to load into the SYST_CVR register when the counter reaches 0

	// Clear the current value of the SysTick timer
	(*(volatile uint32_t *)(0xE000E010U + 0x08)) = 0; // SYST_CVR
	// CURRENT - Current counter value. This is the value of the counter at the time it is sampled

	// Enable the SysTick timer with processor clock and interrupt
	(*(volatile uint32_t *)(0xE000E010U + 0x00)) = (0b111 << 0); // SYST_CSR
	// CLKSOURCE 1 - SysTick uses the processor clock
	// TICKINT   1 - Count to 0 changes the SysTick exception status to pending
	// ENABLE    1 - Counter is operating

	// Wait until the count flag is set
	while (!((*(volatile uint32_t *)(0xE000E010U + 0x00)) & (1 << 16))); // SYST_CSR
	// COUNTFLAG - set 1 if timer has counted to 0.

	// Disable the SysTick timer
	(*(volatile uint32_t *)(0xE000E010U + 0x00)) = 0; // SYST_CSR
}

int main(void)
{
	// Enable GPIOA clock
	(*(volatile uint32_t *)(0x40021000U + 0x14)) |= (1 << 17); // RCC

	// Configure PA7 as output mode
	(*(volatile uint32_t *)(0x48000000U + 0x00)) &= ~(3 << 14); // MODER
	(*(volatile uint32_t *)(0x48000000U + 0x00)) |=  (1 << 14);

	while (1)
	{
		// Toggle PA7 pin
		(*(volatile uint32_t *)(0x48000000U + 0x14)) ^= (1 << 7); // ODR

		// Delay for 500 milliseconds
		_delay_ms(500);
	}
}