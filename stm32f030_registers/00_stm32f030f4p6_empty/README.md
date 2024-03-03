# 00_stm32f030f4p6_empty
# sat 02 mar 2024

At first, I created a simple Main. The file in the main function of which is nothing. I added two files that will help to correctly compile a executable file, see https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/

Сначало я создал простой main.с файл в в основной функции которой ничего нет. Я добавил два файла которые помогут правильно скомпиллировать исполняемый файл, см. https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/

pin 15 - GND | pin 16 - 3V3 | pin 19 - SWDIO | pin 20 - SWCLK

# Compile
arm-none-eabi-as -o crt.o crt.s
# Compile
arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c
# Linker
arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o
# Convert .elf file to .bin
arm-none-eabi-objcopy -O binary main.elf main.bin
# flash STM32F030F4P6
st-flash write main.bin 0x8000000

If the .bin file is compiled, then everything is OK and you can start working with registers directly.

Если .bin файл скомпилирован, то всё ок и можно начинать работать с регистрами напрямую.