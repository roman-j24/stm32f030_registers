// Created wed 15 may 2024
// arm-none-eabi-as -o crt.o crt.s && arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c && arm-none-eabi-gcc -mcpu=cortex-m0 -T linker.ld --specs=nosys.specs -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group -o main.elf crt.o main.o && arm-none-eabi-objcopy -O binary main.elf main.bin && st-flash write main.bin 0x8000000 && rm *.o *.bin *.elf

#include <stdint.h>

#define RCC_AHBENR    (*((volatile uint32_t *)0x40021014))
#define RCC_APB2ENR   (*((volatile uint32_t *)0x40021018))
#define RCC_APB1ENR   (*((volatile uint32_t *)0x4002101C))

#define I2C1_CR1      (*((volatile uint32_t *)0x40005400))
#define I2C1_CR2      (*((volatile uint32_t *)0x40005404))
#define I2C1_OAR1     (*((volatile uint32_t *)0x40005408))
#define I2C1_TIMINGR  (*((volatile uint32_t *)0x40005410))
#define I2C1_ISR      (*((volatile uint32_t *)0x40005418))
#define I2C1_TXDR     (*((volatile uint32_t *)0x40005428))

#define GPIOA_MODER   (*((volatile uint32_t *)0x48000000))
#define GPIOA_OTYPER  (*((volatile uint32_t *)0x48000004))
#define GPIOA_OSPEEDR (*((volatile uint32_t *)0x48000008))
#define GPIOA_ODR     (*((volatile uint32_t *)0x48000014))
#define GPIOA_BSRR    (*((volatile uint32_t *)0x48000018))
#define GPIOA_AFRH    (*((volatile uint32_t *)0x48000024))

void initI2C(uint8_t i2c_addr_dev)
{
	RCC_APB2ENR   |=  (1 << 0);  // SYSCFGEN
	RCC_APB1ENR   |=  (1 << 28); // PWREN
	RCC_AHBENR    |=  (1 << 17); // GPIOAEN
	RCC_APB1ENR   |=  (1 << 21); // i2c clock enable

	GPIOA_MODER   |=  (2 << 18) | (2 << 20);    // Set PA9, PA10 as alternate function mode
	GPIOA_OTYPER  |=  (1 << 9)  | (1 << 10);
	GPIOA_AFRH    &= ~(0xF << 4) & ~(0xF << 8); // Clear bits for PA9, PA10
	GPIOA_AFRH    |=  (4 << 4)  | (4 << 8);     // Clear bits for PA9, PA10
	GPIOA_OSPEEDR |=  (3 << 18) | (3 << 20);

	I2C1_TIMINGR  |=  (0xE << 0);  // SCLL: SCL low period (master mode)
	I2C1_TIMINGR  |=  (0x9 << 8);  // SCLH: SCL high period (master mode)
	I2C1_TIMINGR  |=  (0x2 << 28); // PRESC: Timing prescaler

	I2C1_OAR1     |=  (1 << 15); // own address 1 register

	I2C1_CR2 |= (0 << 25);   // AUTOEND: Automatic end mode (master mode)
	I2C1_CR2 |= (2 << 16);   // NBYTES: Number of bytes
	I2C1_CR2 |= (i2c_addr_dev << 1); // Slave address (master mode)

	I2C1_CR1 |= (1 << 0);    // PE: Peripheral enable
}

void writeI2C(uint8_t reg_addr, uint16_t data)
{
	while ((I2C1_ISR & (0 << 15)) == 1); // BUSY
	while ((I2C1_ISR & (0 << 25)) == 1); // AUTOEND
        I2C1_CR2 |= (1 << 13);               // START: Start generation
        while ((I2C1_ISR & (0 << 1)) == 1);  // wait for tx buffer to be empty
	I2C1_TXDR = reg_addr;                // memory adress to start storing
	while ((I2C1_ISR & (0 << 7)) == 1);  // TCR: Transfer complete reload

	while ((I2C1_ISR & (0 << 1)) == 1);  // wait for tx buffer to be empty/ready
	I2C1_TXDR = data;                    // memory adress to start storing
	while ((I2C1_ISR & (0 << 5)) == 1);  // STOPF: Stop detection flag
	while ((I2C1_ISR & (0 << 6)) == 1);  // TC: Transfer complete (master mode) - all bytes transfered
}

int main()
{
    uint8_t I2C_address_dev = 0x27;
    uint8_t I2C_address_reg = 0x00;

    initI2C(I2C_address_dev);

    while (1)
    {
    	for (uint8_t i=0; i<8; i++)
        {
    	    writeI2C(I2C_address_reg, (1 << i)); // shift 1 from P0 to P7
            for (uint32_t i=0; i<16000*1; i++); // dummy delay
        }
    }

    return 0;
}
