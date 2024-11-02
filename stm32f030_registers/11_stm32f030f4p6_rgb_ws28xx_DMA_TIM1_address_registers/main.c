// Created thu 31 okt 2024
// clear && make all && make clean && make flash
// clear && st-info --probe
// clear && st-flash erase
// clear && arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-gcc -mcpu=cortex-m0 -T linker.ld --specs=nosys.specs -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && arm-none-eabi-size *.elf && rm *.o *.bin *.elf

// https://narodstream.ru/stm-urok-119-ws2812b-lenta-na-umnyx-svetodiodax-rgb-chast-2/

#include <stdint.h>

// Define memory-mapped registers
#define RCC_CR      *(volatile uint32_t *)(0x40021000U)
#define RCC_CFGR    *(volatile uint32_t *)(0x40021004U)
#define RCC_AHBENR  *(volatile uint32_t *)(0x40021014U)
#define RCC_APB2ENR *(volatile uint32_t *)(0x40021018U)

#define GPIOA_MODER *(volatile uint32_t *)(0x48000000U)
#define GPIOA_AFRH  *(volatile uint32_t *)(0x48000024U)

// Timer registers (TIM1)
#define TIM1_CR1    *(volatile uint32_t *)(0x40012C00U)
#define TIM1_DIER   *(volatile uint32_t *)(0x40012C0CU)
#define TIM1_CCMR1  *(volatile uint32_t *)(0x40012C18U)
#define TIM1_CCER   *(volatile uint32_t *)(0x40012C20U)
#define TIM1_ARR    *(volatile uint32_t *)(0x40012C2CU)
#define TIM1_CCR2   *(volatile uint32_t *)(0x40012C38U)
#define TIM1_BDTR   *(volatile uint32_t *)(0x40012C44U)

// DMA registers
#define DMA1_ISR    *(volatile uint32_t *)(0x40020000U)
#define DMA1_IFCR   *(volatile uint32_t *)(0x40020004U)
#define DMA1_CCR3   *(volatile uint32_t *)(0x40020030U)
#define DMA1_CNDTR3 *(volatile uint32_t *)(0x40020034U)
#define DMA1_CPAR3  *(volatile uint32_t *)(0x40020038U)
#define DMA1_CMAR3  *(volatile uint32_t *)(0x4002003CU)

#define NVIC_ISER0  *(volatile uint32_t *)(0xE000E100U) // Interrupt Set Enable Register

#define DELAY_LEN          40
#define DATA_24BIT         24
#define LED_COUNT          1
#define ARRAY_LEN          (DELAY_LEN + LED_COUNT * DATA_24BIT)
#define HIGH               34            // duty
#define LOW                17            // duty LOW = (TIM1_ARR + 1) * 0.35 / 1.25
#define BitIsSet(reg, bit) ((reg & (1 << bit)) != 0) // устанвлен ли bit в 1

uint16_t BUF_DMA[ARRAY_LEN] = {0};

void PLL_INIT(void)
{
    RCC_CFGR |= (0xA << 18);             // 1010: PLL input clock x 12
    RCC_CR   |= (1 << 24) | (1 << 25);   // PLLON, PLLRDY
    RCC_CFGR |= (0x2 << 0);              // SW - HSI selected as system clock
}

void TIM1_INIT(void)
{
    RCC_APB2ENR |= (1 << 11);            // Enable TIM1 clock

    // Configure TIM1
    TIM1_CR1 = 0;                        // Reset CR1
    TIM1_ARR = 60-1;                     // Auto-reload value, 48MHz/60 = 800kHz

    // Configure channel 2 for PWM mode
    TIM1_CCMR1 &= ~(0x7 << 12);          // OC2M, Output Compare 2 mode. Reset
    TIM1_CCMR1 |= (0x6 << 12);           // PWM mode 1
    TIM1_CCMR1 |= (1 << 11);             // OC2PE, Output Compare 2 preload enable

    // Configure Break and Dead-time
    TIM1_BDTR  = 0;
    TIM1_BDTR |= (1 << 13);              // BKP
    TIM1_BDTR |= (1 << 15);              // MOE, Main output enable

    // Configure GPIO (assume PA9 for TIM1_CH2)
    RCC_AHBENR  |= (1 << 17);            // IOPAEN
    GPIOA_MODER &= ~(3 << (9 * 2));
    GPIOA_MODER |= (2 << (9 * 2));       // Alternate function
    GPIOA_AFRH  &= ~(0xF << 4);
    GPIOA_AFRH  |= (2 << 4);             // AF2 (TIM1_CH2)
}

void DMA_INIT(void)
{
    RCC_AHBENR |= (1 << 0);              // Enable DMA1 clock
    NVIC_ISER0 |= (1 << 10);             // Enable DMA1_Channel2_3 interrupt in NVIC
}

void WS28xx_INIT(void)
{
    for (int i = DELAY_LEN; i < ARRAY_LEN; i++)
        BUF_DMA[i] = LOW;
}

void TIM1_PWM_SEND_DMA(void)
{
    DMA1_CCR3   = 0;                     // Disable DMA channel
    DMA1_CPAR3  = (uint32_t)&TIM1_CCR2;  // Peripheral address
    DMA1_CMAR3  = (uint32_t)BUF_DMA;     // Мemory address
    DMA1_CNDTR3 = ARRAY_LEN;             // Number of data to transfer
    DMA1_CCR3 |= (1 << 4);               // DIR, Memory to peripheral
    DMA1_CCR3 |= (1 << 7);               // MINC, Memory increment mode
    DMA1_CCR3 |= (1 << 8);               // PSIZE, Peripheral data size: 16 bits (Half Word) - TIM1 is 16bit
    DMA1_CCR3 |= (1 << 10);              // MSIZE, Memory data size: 16 bits (Half Word)
    DMA1_CCR3 |= (1 << 1);               // TCIE, Enable transfer complete interrupt
    DMA1_CCR3 |= (1 << 0);               // EN, Enable DMA channel

    // Configure TIM1
    TIM1_DIER |= (1 << 10);              // CC2DE, DMA request enable for CCR2
    TIM1_CCER |= (1 << 4);               // CC2E, Enable channel 2 output
    TIM1_CR1  |= (1 << 0);               // CEN, Counter enable
}

void DMA1_Channel2_3_IRQHandler(void)
{
    // Check if transfer complete interrupt is triggered
    if (DMA1_ISR & (1 << 9))
    {
        DMA1_IFCR |= (1 << 9);           // Clear transfer complete flag

        // Stop PWM and DMA
        TIM1_DIER &= ~(1 << 10);         // CC2DE
        DMA1_CCR3 &= ~(1 << 0);          // EN
        TIM1_CCER &= ~(1 << 4);          // CC2E
        TIM1_CR1  &= ~(1 << 0);          // CEN
    }
}

void ws28xx_pixel_rgb_to_buf_dma(uint32_t Rpixel, uint32_t Gpixel, uint32_t Bpixel, uint32_t posX)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        BUF_DMA[DELAY_LEN + posX * 24 + i + 8]  = BitIsSet(Rpixel, (7 - i)) ? HIGH : LOW;
        BUF_DMA[DELAY_LEN + posX * 24 + i + 0]  = BitIsSet(Gpixel, (7 - i)) ? HIGH : LOW;
        BUF_DMA[DELAY_LEN + posX * 24 + i + 16] = BitIsSet(Bpixel, (7 - i)) ? HIGH : LOW;
    }
}

int main(void)
{
    PLL_INIT();
    DMA_INIT();
    TIM1_INIT();
    WS28xx_INIT();

    uint32_t n = 5;

    while (1)
    {
        ws28xx_pixel_rgb_to_buf_dma(254, 0, 0, 0); // RGB
        TIM1_PWM_SEND_DMA();
        for (uint32_t i = 0; i < 65000 * n; i++);

        ws28xx_pixel_rgb_to_buf_dma(0, 254, 0, 0); // RGB
        TIM1_PWM_SEND_DMA();
        for (uint32_t i = 0; i < 65000 * n; i++);

        ws28xx_pixel_rgb_to_buf_dma(0, 0, 254, 0); // RGB
        TIM1_PWM_SEND_DMA();
        for (uint32_t i = 0; i < 65000 * n; i++);
    }
}