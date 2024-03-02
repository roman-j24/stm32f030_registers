// Created sun 25 feb 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000

#include <stdint.h>

int main(void)
{
	// Enable GPIOA, GPIOB, and GPIOF peripheral clocks
	*((volatile uint32_t *)(0x40021000 + 0x14)) |= (1 << 17) | (1 << 18) | (1 << 22);  // RCC_AHBENR, see p.110

	// Set PA0, PB1, and PF0 as output, see p.134 - 8.4.1 GPIO port mode register (GPIOx_MODER)
	// 0x48000000 and offset 0x00 - address port A GPIOx_MODER
	// 0x48000400 and offset 0x00 - address port B GPIOx_MODER
	// 0x48001400 and offset 0x00 - address port F GPIOx_MODER
	*((volatile uint32_t *)(0x48000000 + 0x00)) |= (1 << 0); // PA0, set 1 reg 0
	*((volatile uint32_t *)(0x48000400 + 0x00)) |= (1 << 2); // PB1, set 1 reg 2
	*((volatile uint32_t *)(0x48001400 + 0x00)) |= (1 << 0); // PF0, set 1 reg 0

	while(1)
    	{
        	// Toggle PA0, PB1, and PF0, see p.136 - 8.4.6 GPIO port output data register (GPIOx_ODR)
		// 0x48000400 and offset 0x14 - adddress port A GPIOx_ODR
		*((volatile uint32_t *)(0x48000000 + 0x14)) ^= (1 << 0); // PA0, xor pin 0
		*((volatile uint32_t *)(0x48000400 + 0x14)) ^= (1 << 1); // PB1, xor pin 1
		*((volatile uint32_t *)(0x48001400 + 0x14)) ^= (1 << 0); // PF0, xor pin 0

		for (uint32_t i = 0; i < 32000*2; i++); // dummy delay
	}
}