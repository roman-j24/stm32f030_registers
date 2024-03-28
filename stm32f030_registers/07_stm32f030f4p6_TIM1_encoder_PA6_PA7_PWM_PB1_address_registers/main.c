// Created thu 28 mar 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-gcc -mcpu=cortex-m0 -T linker.ld --specs=nosys.specs -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && rm *.o *.bin *.elf

#include <stdint.h>

// Define GPIO registers addresses
#define RCC_AHBENR  (*((volatile uint32_t *)0x40021014))
#define RCC_APB2ENR (*((volatile uint32_t *)0x40021018))
#define RCC_APB1ENR (*((volatile uint32_t *)0x4002101C))

#define GPIOA_MODER (*((volatile uint32_t *)0x48000000))
#define GPIOA_AFRL  (*((volatile uint32_t *)0x48000020))
#define GPIOA_AFRH  (*((volatile uint32_t *)0x48000024))
#define GPIOA_PUPDR (*((volatile uint32_t *)0x4800040C))

#define TIM1_CR1    (*((volatile uint32_t *)0x40012C00))
#define TIM1_ARR    (*((volatile uint32_t *)0x40012C2C))
#define TIM1_CCMR1  (*((volatile uint32_t *)0x40012C18))
#define TIM1_CCER   (*((volatile uint32_t *)0x40012C20))
#define TIM1_PSC    (*((volatile uint32_t *)0x40012C28))
#define TIM1_CCR2   (*((volatile uint32_t *)0x40012C38))
#define TIM1_BDTR   (*((volatile uint32_t *)0x40012C44))

#define TIM3_CR1    (*((volatile uint32_t *)0x40000400))
#define TIM3_SMCR   (*((volatile uint32_t *)0x40000408))
#define TIM3_ARR    (*((volatile uint32_t *)0x4000042C))
#define TIM3_CCMR1  (*((volatile uint32_t *)0x40000418))
#define TIM3_CCER   (*((volatile uint32_t *)0x40000420))
#define TIM3_CNT    (*((volatile uint32_t *)0x40000424))

int main()
{
	RCC_AHBENR  |=  (1 << 17);            // GPIOAEN

	// Configure PA9, TIM1 - PWM
	GPIOA_AFRH  &= ~(0xF << 4);           // Clear bits for PA9
	GPIOA_AFRH  |=  (2 << 4);             // Set AF2 for PA9 (TIM1_CH2)
	GPIOA_MODER |=  (0b10 << 18);         // Set PA9 as alternate function mode
	RCC_APB2ENR |=  (1 << 11);            // TIM1EN
	// Frequency, duty pulse
	TIM1_ARR     = 1000 - 1;	      // Set auto-reload value
	TIM1_PSC     = 80 - 1;		      // Set prescaler value
	TIM1_CCMR1  |= (6 << 12);             // Set PWM mode 1 for channel 2
	// Output and counter enable
	TIM1_CCER   |= (1 << 4);              // CC2E: Capture/Compare 2 output enable
	TIM1_BDTR   |= (1 << 15);             // MOE: Main output enable
	TIM1_CR1    |= (1 << 0);	      // CEN: Counter enable

	// Configure PA6, PA7, TIM3 - ENCODER
	GPIOA_PUPDR |= (1 << 12) | (1 << 14);
	GPIOA_AFRL  |= (0b001 << 24) | (0b001 << 28); // Set AF1 for PA6-TIM3_CH1, PA7-TIM3_CH2
	GPIOA_MODER |= (2 << 12) | (2 << 14);         // Set PA6, PA7 as alternate function mode
	RCC_APB1ENR |= (1 << 1);              // TIM3EN
	TIM3_ARR     = 1000 - 1;	      // Set auto-reload value
	TIM3_SMCR   |= (3 << 0);	      // SMS: Slave mode selection
	TIM3_CCMR1  |= (1 << 0) | (1 << 8);   // CC1S, CC2S
	// Output and counter enable
	TIM3_CCER   |= (1 << 0) | (1 << 4);   // CC2E
	TIM3_CR1    |= (1 << 0);	      // CEN: Counter enable

	while (1)
	{
		if (TIM3_CNT >= (100 - 1))
			TIM3_CNT = (100 - 1);

		if (TIM3_CNT <= 0)
			TIM3_CNT = 1;

		TIM1_CCR2 = TIM3_CNT * 10;    // duty,  step 10%
	}

	return 0;
}