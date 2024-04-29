################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/boot_multicore_slave.c \
../startup/startup_lpc55s69_cm33_core0.c 

C_DEPS += \
./startup/boot_multicore_slave.d \
./startup/startup_lpc55s69_cm33_core0.d 

OBJS += \
./startup/boot_multicore_slave.o \
./startup/startup_lpc55s69_cm33_core0.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.c startup/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DMBEDTLS_FREESCALE_HASHCRYPT_SHA256 -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=1 -DSDK_DEBUGCONSOLE_UART -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\drivers" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\device" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\CMSIS" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\utilities" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\component\uart" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\component\serial_manager" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\mbedtls\include" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\mbedtls\library" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\mbedtls\port\ksdk" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\component\lists" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\board" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\startup" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\freertos\corejson\source\include" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\LPC55S69_PAWOS\source" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-startup

clean-startup:
	-$(RM) ./startup/boot_multicore_slave.d ./startup/boot_multicore_slave.o ./startup/startup_lpc55s69_cm33_core0.d ./startup/startup_lpc55s69_cm33_core0.o

.PHONY: clean-startup

