# 09_stm32f030f4p6_I2C_PCF8574_LCD1602_address_registers
# wed 19 may 2024

# pinout STM32F030F4P6 
pin 15 - GND | 
pin 16 - 3V3 | 
pin 19 - SWDIO | 
pin 20 - SWCLK 

PA9  - SCL | 
PA10 - SDA | 

YwRobot Arduino LCM1602 IIC V1,
PCF8574 - Remote 8-Bit I/O Expander for I2C Bus. 

P0 - RS - 4  pin 
P1 - RW - 5  pin 
P2 - EN - 6  pin 
P3 - BT - transistors (base) S8050 
P4 - D4 - 11 pin 
P5 - D5 - 12 pin 
P6 - D6 - 13 pin 
P7 - D7 - 14 pin 

pin 6 - GND | pin 16 - +3.3V or V+5V

The display is controlled by 8 wires. Three wires are responsible for the control mode, four wires transmit information that must be printed. And the eighth wire answers the back light of the display.

4 -bit mode is used, so 8 bit data are divided into two halves and sent in parts.

On my version of the board there are no resistors pulling up to the plus for i2c, the poet I had to insert them on the layout.

https://www.youtube.com/watch?v=njukunLdQ1Y
