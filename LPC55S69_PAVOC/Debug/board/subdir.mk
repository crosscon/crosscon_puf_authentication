################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/peripherals.c \
../board/pin_mux.c 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/peripherals.d \
./board/pin_mux.d 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/peripherals.o \
./board/pin_mux.o 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c board/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DMBEDTLS_FREESCALE_HASHCRYPT_SHA256 -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=1 -DSDK_DEBUGCONSOLE_UART -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\drivers" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\device" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\CMSIS" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\utilities" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\component\uart" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\component\serial_manager" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\mbedtls\include" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\mbedtls\library" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\mbedtls\port\ksdk" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\component\lists" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\board" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\startup" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\freertos\corejson\source\include" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAVOC\source" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-board

clean-board:
	-$(RM) ./board/board.d ./board/board.o ./board/clock_config.d ./board/clock_config.o ./board/peripherals.d ./board/peripherals.o ./board/pin_mux.d ./board/pin_mux.o

.PHONY: clean-board

