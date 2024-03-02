# 00_stm32f030f4p6_empty
# sat 02 mar 2024

At first, I created a simple Main. The file in the main function of which is nothing. I added two files that will help to correctly compile a executable file, see https://kleinmbeded.com/stm32-without-cubeide-part-the-the-necessites/

Сначало я создал простой main.с файл в в основной функции которой ничего нет. Я добавил два файла которые помогут правильно скомпиллировать исполняемый файл, см. https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/

arm-none-eabi-as -o crt.o crt.s
arm-none-eabi-gcc -mthumb -mcpu=cortex-m0 -O0 -c -o main.o main.c
arm-none-eabi-ld -T linker.ld -o main.elf crt.o main.o
arm-none-eabi-objcopy -O binary main.elf main.bin
st-flash write main.bin 0x8000000

If the .bin file is compiled, then everything is OK and you can start working with registers directly.

Если .bin файл скомпилирован, то всё ок и можно начинать работать с регистрами напрямую.