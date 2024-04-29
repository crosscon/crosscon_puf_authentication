################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/fsl_assert.c \
../utilities/fsl_debug_console.c \
../utilities/fsl_str.c 

C_DEPS += \
./utilities/fsl_assert.d \
./utilities/fsl_debug_console.d \
./utilities/fsl_str.d 

OBJS += \
./utilities/fsl_assert.o \
./utilities/fsl_debug_console.o \
./utilities/fsl_str.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/%.o: ../utilities/%.c utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DPRINTF_ADVANCED_ENABLE=1 -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -D__REDLIB__ -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\board" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\source" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\drivers" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\device" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\CMSIS" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\utilities" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\component\uart" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\component\serial_manager" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\mbedtls\include" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\mbedtls\library" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\mbedtls\port\ksdk" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\component\lists" -I"C:\Users\lup22vs\Desktop\CROSSCON\PUF-authentication\PUF_Intern_Project\startup" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities

clean-utilities:
	-$(RM) ./utilities/fsl_assert.d ./utilities/fsl_assert.o ./utilities/fsl_debug_console.d ./utilities/fsl_debug_console.o ./utilities/fsl_str.d ./utilities/fsl_str.o

.PHONY: clean-utilities

