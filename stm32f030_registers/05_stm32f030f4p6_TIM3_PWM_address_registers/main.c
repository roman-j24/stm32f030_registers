// Created mon 11 mar 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && rm *.o *.bin *.elf

// see p.34 Table 12. Alternate functions selected through GPIOA_AFR registers for port A "stm32f030f4.pdf" DS9773 Rev 5

#include <stdint.h>

#define DUTY_PERCENT 75
#define DUTY_VALUE ((1000 - 1) * DUTY_PERCENT) / 100 - 1

int main(void)
{
	// Configure TIM3, enable clock
	(*(volatile uint32_t *)(0x40021000U + 0x1C)) |= (1 << 1);      // RCC_APB1ENR
	// Configure TIM3, frequency, duty pulse
	(*(volatile uint32_t *)(0x40000400U + 0x2C)) = 1000 - 1;       // TIM3_ARR, set auto-reload value
	(*(volatile uint32_t *)(0x40000400U + 0x28)) = 80 - 1;	       // TIM3_PSC, set prescaler value
	(*(volatile uint32_t *)(0x40000400U + 0x38)) = DUTY_VALUE;     // TIM3_CCR2, duty
	// Configure TIM3, PWM
	(*(volatile uint32_t *)(0x40000400U + 0x18)) |= (0b110 << 12); // TIM3_CCMR1, Set PWM mode 1 for channel 2

	// Configure PA7 as AF
	(*(volatile uint32_t *)(0x40021000U + 0x14)) |= (1 << 17);     // RCC_AHBENR, Enable GPIOA clock
	(*(volatile uint32_t *)(0x48000000U + 0x20)) &= ~(0xF << 28);  // GPIOA_AFRL, Clear bits for PA7
	(*(volatile uint32_t *)(0x48000000U + 0x20)) |= (1 << 28);     // GPIOA_AFRL, Set alternate function 1 for PA7 (TIM3_CH2)
	(*(volatile uint32_t *)(0x48000000U + 0x00)) |= (2 << 14);     // GPIOA_MODER, Set PA7 as alternate function mode
	
	// Configure TIM3, output and counter enable
	(*(volatile uint32_t *)(0x40000400U + 0x20)) |= (1 << 4);      // TIM3_CCER, CC2E: Capture/Compare 2 output enable
	(*(volatile uint32_t *)(0x40000400U + 0x00)) |= (1 << 0);      // TIM3_CR1, CEN: Counter enable

	// Configure PB1 as output mode
	(*(volatile uint32_t *)(0x40021000U + 0x14)) |= (1 << 18);     // RCC_AHBENR, Enable GPIOB clock
	*((volatile uint32_t *)(0x48000400U + 0x00)) |= (1 << 2);      // MODER PB1

	while (1)
	{
		// PB1 blink
		*((volatile uint32_t *)(0x48000400U + 0x14)) ^= (1 << 1); // ODR PB1
		for (uint32_t i = 0; i < 32000 * 2; i++); //*/
	}
}