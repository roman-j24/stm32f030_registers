// Created tue 04 jun 2024
// clear && st-info --probe
// clear && st-flash erase
// clear && arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-gcc -mcpu=cortex-m0 -T linker.ld --specs=nosys.specs -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && arm-none-eabi-size *.elf && rm *.o *.bin *.elf

#include <stdint.h>

#define SYSTICK_CTRL  (*(volatile unsigned int*)0xE000E010)
#define SYSTICK_LOAD  (*(volatile unsigned int*)0xE000E014)
#define SYSTICK_VAL   (*(volatile unsigned int*)0xE000E018)
#define SYSTICK_CALIB (*(volatile unsigned int*)0xE000E01C)

#define STK_CTRL_ENABLE    0x1
#define STK_CTRL_TICKINT   0x2
#define STK_CTRL_CLKSOURCE 0x4
#define STK_CTRL_COUNTFLAG 0x10000

#define RCC_APB1ENR   (*(volatile uint32_t *)0x4002101C)
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40021018)
#define RCC_AHBENR    (*(volatile uint32_t *)0x40021014)

#define GPIOA_MODER   (*(volatile uint32_t *)0x48000000)
#define GPIOA_OTYPER  (*(volatile uint32_t *)0x48000004)
#define GPIOA_OSPEEDR (*(volatile uint32_t *)0x48000008)
#define GPIOA_AFRL    (*(volatile uint32_t *)0x48000020)
#define GPIOA_AFRH    (*(volatile uint32_t *)0x48000024)

#define USART1_CR1    (*(volatile uint32_t *)0x40013800)
#define USART1_BRR    (*(volatile uint32_t *)0x4001380C)
#define USART1_ISR    (*(volatile uint32_t *)0x4001381C)
#define USART1_TDR    (*(volatile uint32_t *)0x40013828)

#define I2C1_CR1      (*(volatile uint32_t *)0x40005400)
#define I2C1_CR2      (*(volatile uint32_t *)0x40005404)
#define I2C1_OAR1     (*(volatile uint32_t *)0x40005408)
#define I2C1_TIMINGR  (*(volatile uint32_t *)0x40005410)
#define I2C1_ISR      (*(volatile uint32_t *)0x40005418)
#define I2C1_ICR      (*(volatile uint32_t *)0x4000541C)
#define I2C1_TXDR     (*(volatile uint32_t *)0x40005428)
#define I2C1_RXDR     (*(volatile uint32_t *)0x40005424)

#define DS3231_ADDRESS 0x68

// Global variable for reference milliseconds
volatile unsigned int msTicks = 0;

// Systick interrupt handler
void SysTick_Handler(void)
{
    msTicks++;
}

// Init Systick Timer to generate interruption every 1 ms
void SysTick_Init(void)
{
    SYSTICK_LOAD = (8000000 - 1)/1000;
    SYSTICK_VAL  = 0;                                                       // Reset the current value
    SYSTICK_CTRL = STK_CTRL_ENABLE | STK_CTRL_TICKINT | STK_CTRL_CLKSOURCE; // Turn on SysTick
}

// Function for delay without blocking
void _delay_ms(unsigned int delay)
{
    unsigned int startTicks = msTicks;
    while ((msTicks - startTicks) < delay);
}

void Init_UART(void)
{
    RCC_AHBENR  |= (1 << 17);                      // Enable GPIOA clock
    GPIOA_MODER |= (2 << 4);                       // Configure PA2, PA3 as alternate function mode
    GPIOA_MODER |= (2 << 6);
    GPIOA_AFRL  |= (1 << 8) | (1 << 12);           // GPIOx_AFRL for AF1 (USART1)
    RCC_APB2ENR |= (1 << 14);                      // Enable USART1 clock
    USART1_BRR = 8000000 / 9600;                   // Configure USART1 baud rate to 9600 (8MHz)
    USART1_CR1  |= (1 << 0) | (1 << 2) | (1 << 3); // Enable transmitter and USART1 UE - usart enable, RE - rx enable, TE - tx enable
}

void Init_I2C(void)
{
    RCC_APB2ENR |= (1 << 0);   // SYSCFGEN
    RCC_APB1ENR |= (1 << 28);  // PWREN
    RCC_AHBENR  |= (1 << 17);  // GPIOAEN
    RCC_APB1ENR |= (1 << 21);  // I2C1 clock enable

    GPIOA_MODER   |= (2 << 18) | (2 << 20);     // Set PA9, PA10 as alternate function mode
    GPIOA_OTYPER  |= (1 << 9) | (1 << 10);
    GPIOA_AFRH    &= ~(0xF << 4) & ~(0xF << 8); // Clear bits for PA9, PA10
    GPIOA_AFRH    |= (4 << 4) | (4 << 8);
    GPIOA_OSPEEDR |= (3 << 18) | (3 << 20);

    I2C1_TIMINGR |= (0xE << 0);  // SCLL: SCL low period (master mode)
    I2C1_TIMINGR |= (0x9 << 8);  // SCLH: SCL high period (master mode)
    I2C1_TIMINGR |= (0x2 << 28); // PRESC: Timing prescaler

    I2C1_OAR1 |= (1 << 15);      // OA1EN: Own address 1 enable
    I2C1_CR2  |= (1 << 25);      // AUTOEND: Automatic end mode (master mode)
    I2C1_CR1  |= (1 << 0);       // PE: Peripheral enable
}

uint32_t I2C1_Read(uint32_t reg)
{
    while ((I2C1_ISR & (0 << 15)));    // Wait until BUSY flag is reset
    while ((I2C1_ISR & (0 << 25)));    // Wait until AUTOEND flag is reset

    I2C1_CR2 |= (1 << 16);             // NBYTES: Number of bytes, 1 byte
    I2C1_CR2 |= (DS3231_ADDRESS << 1); // Slave address (master mode)
    I2C1_CR2 &= ~(1 << 10);            // RD_WRN: Master requests a WRITE transfer
    I2C1_CR2 |= (1 << 13);             // START: Start generation
    while (!(I2C1_ISR & (1 << 1)));    // Wait until TXIS is set
    I2C1_TXDR = reg;
    while ((I2C1_ISR & (0 << 1)));     // Wait until TXIS flag is reset

    I2C1_CR2 |= (1 << 25);             // AUTOEND
    I2C1_CR2 |= (1 << 10);             // RD_WRN: Master requests a READ transfer
    I2C1_CR2 |= (1 << 13);             // START: Start generation
    while (!(I2C1_ISR & (1 << 2)));    // Wait until RXNE flag is set
    uint32_t data = I2C1_RXDR;
    while ((I2C1_ISR & (0 << 2)));     // Wait until RXNE flag is reset

    I2C1_CR2 = 0;                      // Clear I2C1_CR2
    return data;
}

void I2C1_Write(uint32_t reg, uint32_t data)
{
    while ((I2C1_ISR & (0 << 15)));    // Wait until BUSY flag is reset
    while ((I2C1_ISR & (0 << 25)));    // Wait until AUTOEND flag is reset

    I2C1_CR2 |= (2 << 16);             // NBYTES: Number of bytes, 1 byte
    I2C1_CR2 |= (DS3231_ADDRESS << 1); // Slave address (master mode)
    I2C1_CR2 &= ~(1 << 10);            // RD_WRN: Master requests a WRITE transfer
    I2C1_CR2 |= (1 << 13);             // START: Start generation
    while (!(I2C1_ISR & (1 << 1)));    // Wait until TXIS is set
    I2C1_TXDR = reg;
    while ((I2C1_ISR & (0 << 1)));     // Wait until TXIS flag is reset

    I2C1_CR2 |= (1 << 25);             // AUTOEND
    I2C1_CR2 &= ~(1 << 10);            // RD_WRN: Master requests a WRITE transfer
    I2C1_CR2 |= (1 << 13);             // START: Start generation
    while (!(I2C1_ISR & (1 << 1)));    // Wait until RTNE flag is set
    I2C1_TXDR = data;
    while ((I2C1_ISR & (0 << 1)));     // Wait until RTNE flag is reset

    I2C1_CR2 = 0;                      // Clear I2C1_CR2
}

void uart_send_char(char c)
{
    while (!(USART1_ISR & (1 << 7)));  // Wait until TXE (Transmit Data Register Empty)
    USART1_TDR = c;
}

void uart_send_string(const char *str)
{
    while (*str)
        uart_send_char(*str++);
}

void int_to_hex_str(uint32_t num, char *str)
{
    const char hex_chars[] = "0123456789ABCDEF";

    for (uint32_t i = 0; i < 2; i++)
    {                                      // Assuming 32-bit integer, which results in 8 hex characters
        str[1 - i] = hex_chars[num & 0xF]; // Get the last 4 bits of num
        num >>= 4;                         // Shift right by 4 bits to get the next hex digit
    }
    str[2] = '\0'; // Null-terminate the string
}

uint32_t BCD_to_Decimal(uint32_t bcd)
{
    return ((bcd / 16) * 10) + (bcd % 16);
}

uint32_t Decimal_to_BCD(uint32_t decimal)
{
    return ((decimal / 10) * 16) + (decimal % 10);
}

void print_ds3231_time(void)
{
    uint32_t seconds = I2C1_Read(0x00);
    uint32_t minutes = I2C1_Read(0x01);
    uint32_t hours   = I2C1_Read(0x02);

    char time_str[9];
    time_str[0] = '0' + ((hours >> 4) & 0x0F);
    time_str[1] = '0' + (hours & 0x0F);
    time_str[2] = ':';
    time_str[3] = '0' + ((minutes >> 4) & 0x0F);
    time_str[4] = '0' + (minutes & 0x0F);
    time_str[5] = ':';
    time_str[6] = '0' + ((seconds >> 4) & 0x0F);
    time_str[7] = '0' + (seconds & 0x0F);
    time_str[8] = '\0';

    uart_send_string(time_str);
    uart_send_string("\r\n");
}

void set_time_ds3231(uint32_t sec, uint32_t min, uint32_t hour)
{
    if (sec < 0 || min < 0 || hour < 0)
        sec = 0, min = 0, hour = 0;

    if (sec > 59) sec = 59;    
    if (min > 59) min = 59;    
    if (hour > 23) hour = 23;
    
    I2C1_Write(0x00, Decimal_to_BCD(sec));  // sec
    I2C1_Write(0x01, Decimal_to_BCD(min));  // min
    I2C1_Write(0x02, Decimal_to_BCD(hour)); // hour
}

int main(void)
{
    SysTick_Init();
    Init_UART();
    Init_I2C();

    uart_send_string("Time from ds3231 I2C...\r\n");

    set_time_ds3231(34, 16, 22); // sec, min, hour

    while (1)
    {
        print_ds3231_time();
        _delay_ms(333); // delay without blocking
    }
}
