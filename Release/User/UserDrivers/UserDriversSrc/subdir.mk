################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/UserDrivers/UserDriversSrc/ButtonsDriver.c \
../User/UserDrivers/UserDriversSrc/LedsDriver.c 

OBJS += \
./User/UserDrivers/UserDriversSrc/ButtonsDriver.o \
./User/UserDrivers/UserDriversSrc/LedsDriver.o 

C_DEPS += \
./User/UserDrivers/UserDriversSrc/ButtonsDriver.d \
./User/UserDrivers/UserDriversSrc/LedsDriver.d 


# Each subdirectory must supply rules for building sources it contributes
User/UserDrivers/UserDriversSrc/%.o User/UserDrivers/UserDriversSrc/%.su User/UserDrivers/UserDriversSrc/%.cyclo: ../User/UserDrivers/UserDriversSrc/%.c User/UserDrivers/UserDriversSrc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/config" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/include" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/portable/GCC/ARM_CM3" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/portable/MemMang" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/source" -I"D:/stm32cubeide workspace/GameConsole/User/UserDrivers/UserDriversInc" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode/UserCodeInc" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode/UserCodeInc/UserTasksFreeRTOS_Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-UserDrivers-2f-UserDriversSrc

clean-User-2f-UserDrivers-2f-UserDriversSrc:
	-$(RM) ./User/UserDrivers/UserDriversSrc/ButtonsDriver.cyclo ./User/UserDrivers/UserDriversSrc/ButtonsDriver.d ./User/UserDrivers/UserDriversSrc/ButtonsDriver.o ./User/UserDrivers/UserDriversSrc/ButtonsDriver.su ./User/UserDrivers/UserDriversSrc/LedsDriver.cyclo ./User/UserDrivers/UserDriversSrc/LedsDriver.d ./User/UserDrivers/UserDriversSrc/LedsDriver.o ./User/UserDrivers/UserDriversSrc/LedsDriver.su

.PHONY: clean-User-2f-UserDrivers-2f-UserDriversSrc

