// Created sat 18 may 2024
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

void initI2C(void)
{
	RCC_APB2ENR   |=  (1 << 0);  // SYSCFGEN
	RCC_APB1ENR   |=  (1 << 28); // PWREN
	RCC_AHBENR    |=  (1 << 17); // GPIOAEN
	RCC_APB1ENR   |=  (1 << 21); // i2c clock enable

	// Configure pins I2C
	GPIOA_MODER   |=  (2 << 18) | (2 << 20);    // Set PA9, PA10 as alternate function mode
	GPIOA_OTYPER  |=  (1 << 9)  | (1 << 10);
	GPIOA_AFRH    &= ~(0xF << 4) & ~(0xF << 8); // Clear bits for PA9, PA10
	GPIOA_AFRH    |=  (4 << 4)  | (4 << 8);     // Clear bits for PA9, PA10
	GPIOA_OSPEEDR |=  (3 << 18) | (3 << 20);

	I2C1_TIMINGR  |=  (0xE << 0); // SCLL: SCL low period (master mode)
	I2C1_TIMINGR  |=  (0x9 << 8); // SCLH: SCL high period (master mode)
	I2C1_TIMINGR  |=  (0x2 << 28); // PRESC: Timing prescaler

	I2C1_OAR1     |=  (1 << 15); // own address 1 register

	I2C1_CR2 |= (0 << 25); // AUTOEND: Automatic end mode (master mode)
	I2C1_CR2 |= (2 << 16); // NBYTES: Number of bytes
	I2C1_CR2 |= (0x27 << 1); // Slave address (master mode)

	I2C1_CR1 |= (1 << 0);  // PE: Peripheral enable
}

void writeI2C(uint32_t reg_addr, uint32_t data)
{
	while ((I2C1_ISR & (0 << 15)) == 1); // BUSY
	while ((I2C1_ISR & (0 << 25)) == 1); // AUTOEND
    	I2C1_CR2 |= (1 << 13);                   // send start condition
    	while ((I2C1_ISR & (0 << 1)) == 1); // wait for tx buffer to be empty/ready
	I2C1_TXDR = reg_addr;                                   // memory adress to start storing
	while ((I2C1_ISR & (0 << 7)) == 1); // TCR: Transfer write_LCDplete reload

	while ((I2C1_ISR & (0 << 1)) == 1); // wait for tx buffer to be empty/ready
	I2C1_TXDR = data;                    // memory adress to start storing
	while ((I2C1_ISR & (0 << 5)) == 1); // STOPF: Stop detection flag
	while ((I2C1_ISR & (0 << 6)) == 1); // TC: Transfer write_LCDplete (master mode) - all bytes transfered
}

void write_LCD(uint32_t data)
{
	data = data | 0b00001100;
	for (uint32_t i=0; i<500; i++);
	writeI2C(0x00, data);  // write data, Е set
	for (uint32_t i=0; i<500; i++);
	writeI2C(0x00, (data & 0b11111011));  // write data, Е unset
}

void initLCD(void)
{
	write_LCD(0b00110000); // 4-Bit Interface
	write_LCD(0b00110000);
	write_LCD(0b00110000);

	write_LCD(0b00100000);

	write_LCD(0b00100000);	// see table 6
	write_LCD(0b10000000);	// 0 0 1 DL N F - -

	write_LCD(0);          // off lcd
	write_LCD(0b00010000);

	write_LCD(0);          // clear lcd
	write_LCD(0b00010000);

	write_LCD(0);          // set mode entry
	write_LCD(0b01100000);

	write_LCD(0);          // on lcd
	write_LCD(0b11000000); // on lcd
}

void print_char(uint32_t data_ch)
{
	uint32_t data_hi = ((data_ch & 0xF0) + 0b00001001);
	uint32_t data_low = ((data_ch << 4) + 0b00001001);

	writeI2C(0x00, data_hi); // send MSB 4 bit
	for (uint32_t i=0; i<200; i++);
	data_hi |= 0b00000100;
	writeI2C(0x00, data_hi);
	data_hi &= 0b11111001;
	writeI2C(0x00, data_hi);

	writeI2C(0x00, data_low); // send LSB 4 bit
	for (uint32_t i=0; i<200; i++);
	data_low |= 0b00000100;
	writeI2C(0x00, data_low);
	data_low &= 0b11111001;
	writeI2C(0x00, data_low);
}

void str_out(char *str)
{
	while (*str)
		print_char(*str++);
}

int main()
{
    initI2C();
	initLCD();

	print_char('$');
	print_char('a');
	print_char('R');
	print_char('\0');

	for (uint32_t i=0; i<64000*12; i++); // dummy delay

	// 4 -bit mode is used, so 8 bit data are divided into two halves and sent in parts
	write_LCD(0); // clear lcd
	write_LCD(0b00010000);

	write_LCD(0); // lcd on, set blink cursor
	write_LCD(0b11110000);

	write_LCD(0b00010000); // first shift right
	write_LCD(0b01000000); // 0 0 0 S/C R/L 1 - -
	write_LCD(0b00010000); // second shift right
	write_LCD(0b01000000);

	str_out("hallo, salut"); // print string and...
	// ... and a dumb gap 40 times to get on the second line. The number of interventions depends on the first line
	for (uint32_t i=0; i<40; i++) // dummy counter
	{
		write_LCD(0b00010000);
		write_LCD(0b01000000);
	}

	for (uint32_t i=0; i<14; i++) // dummy counter
	{
		write_LCD(0b00010000); // shift left i-times
		write_LCD(0b00000000);
		for (uint32_t i=0; i<32000*4; i++); // dummy delay
	}
	str_out("$ in noo seek # "); //*/

	/*write_LCD(0b00010000); // shift left i-times
	write_LCD(0b00000000);
	print_char('$');
	print_char('\0');//*/

	while (1)
	{
		//
    }

    return 0;
}
