// Created mon 25 mar 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-gcc -mcpu=cortex-m0 -T linker.ld --specs=nosys.specs -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && rm *.o *.bin *.elf

#include <stdint.h>

#define RCC_AHBENR  (*((volatile uint32_t *)0x40021014))
#define RCC_APB2ENR (*((volatile uint32_t *)0x40021018))
#define RCC_APB1ENR (*(volatile uint32_t *)(0x4002101C))

#define GPIOA_MODER (*((volatile uint32_t *)0x48000000))
#define GPIOA_AFRL  (*((volatile uint32_t *)0x48000020))
#define GPIOB_MODER (*((volatile uint32_t *)0x48000400))
#define GPIOB_AFRL  (*((volatile uint32_t *)0x48000420))
#define GPIOB_ODR   (*((volatile uint32_t *)0x48000414))

#define TIM3_CR1    (*(volatile uint32_t *)(0x40000400))
#define TIM3_ARR    (*(volatile uint32_t *)(0x4000042C))
#define TIM3_CCMR2  (*(volatile uint32_t *)(0x4000041C))
#define TIM3_CCER   (*(volatile uint32_t *)(0x40000420))
#define TIM3_PSC    (*(volatile uint32_t *)(0x40000428))
#define TIM3_CCR4   (*(volatile uint32_t *)(0x40000440))

#define ADC_ISR     (*((volatile uint32_t *)0x40012400))
#define ADC_CR      (*((volatile uint32_t *)0x40012408))
#define ADC_SMPR    (*((volatile uint32_t *)0x40012414))
#define ADC_CHSELR  (*((volatile uint32_t *)0x40012428))
#define ADC_DR      (*((volatile uint32_t *)0x40012440))

uint32_t val_adc = 0;

int main(void)
{
	RCC_AHBENR  |=  (1 << 17) | (1 << 18);   // GPIOAEN, GPIOBEN
	// Configure PA5
	GPIOB_MODER |=  (0b11 << 10);            // Set PA5 to analog mode
	// Configure PB1
	GPIOB_AFRL  &= ~(0xF << 4);		 // GPIOB_AFRL, Clear bits for PB1
	GPIOB_AFRL  |=  (1 << 4);		 // GPIOB_AFRL, Set alternate function 1 for PB1 (TIM3_CH4)
	GPIOB_MODER |=  (2 << 2);		 // GPIOB_MODER, Set PB1 as alternate function mode

	// Configure TIM3, enable clock
	RCC_APB1ENR |=  (1 << 1);                // RCC_APB1ENR
	// Frequency, duty pulse
	TIM3_ARR     =  1000 - 1;		 // TIM3_ARR, set auto-reload value
	TIM3_PSC     =  80 - 1;			 // TIM3_PSC, set prescaler value
	TIM3_CCR4    =  10;			 // TIM3_CCR4, duty
	TIM3_CCMR2  |=  (0b110 << 12);           // TIM3_CCMR1, Set PWM mode 1 for channel 4
	// Output and counter enable
	TIM3_CCER   |=  (1 << 12);		 // TIM3_CCER, CC4E: Capture/Compare 4 output enable
	TIM3_CR1    |=  (1 << 0);		 // TIM3_CR1, CEN: Counter enable

	// Configure ADC
	RCC_APB2ENR |=  (1 << 9);                // ADCEN
	ADC_CR      |=  (1 << 4) | (1 << 0);     // ADSTP: ADC stop conversion command; ADEN
    	ADC_SMPR    &= ~(0b111 << 0);            // reset
    	ADC_SMPR    |=  (0b111 << 0);            // SMP[2:0]: Sampling time selection
    	ADC_CHSELR  |=  (1 << 5);                // Select channel 5 for conversion
    	ADC_CR      |=  (1 << 31);               // Start calibration
    	while ((ADC_CR & (1 << 31)) != 0);       // Wait until ADCAL=0
    	ADC_CR      |=  (1 << 0);                // ADEN: ADC enable command

	while (1)
	{
		ADC_CR  |=  (1 << 2);		 // Start the ADC conversion
		while ((ADC_ISR & 1 << 2) == 0); // Wait end of conversion
		val_adc = ADC_DR;		 // ADC conversion result

		if (val_adc >= 4000 )	TIM3_CCR4 = 900;
		if (val_adc >= 3800 &  val_adc < 4000)	TIM3_CCR4 = 400;
		if (val_adc >= 2800 &  val_adc < 3800)	TIM3_CCR4 = 200;
		if (val_adc >= 1000 &  val_adc < 2800)	TIM3_CCR4 = 100;
		if (val_adc >= 500 &  val_adc < 1000)	TIM3_CCR4 = 50;
	}

	return 0;
}
