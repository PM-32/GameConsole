################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/UserCode/UserCodeSrc/ButtonsProcessing.c \
../User/UserCode/UserCodeSrc/CommonFreeRTOS.c \
../User/UserCode/UserCodeSrc/HooksFreeRTOS.c \
../User/UserCode/UserCodeSrc/InterruptsPriorities.c \
../User/UserCode/UserCodeSrc/UserTimer.c 

OBJS += \
./User/UserCode/UserCodeSrc/ButtonsProcessing.o \
./User/UserCode/UserCodeSrc/CommonFreeRTOS.o \
./User/UserCode/UserCodeSrc/HooksFreeRTOS.o \
./User/UserCode/UserCodeSrc/InterruptsPriorities.o \
./User/UserCode/UserCodeSrc/UserTimer.o 

C_DEPS += \
./User/UserCode/UserCodeSrc/ButtonsProcessing.d \
./User/UserCode/UserCodeSrc/CommonFreeRTOS.d \
./User/UserCode/UserCodeSrc/HooksFreeRTOS.d \
./User/UserCode/UserCodeSrc/InterruptsPriorities.d \
./User/UserCode/UserCodeSrc/UserTimer.d 


# Each subdirectory must supply rules for building sources it contributes
User/UserCode/UserCodeSrc/%.o User/UserCode/UserCodeSrc/%.su User/UserCode/UserCodeSrc/%.cyclo: ../User/UserCode/UserCodeSrc/%.c User/UserCode/UserCodeSrc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/include" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/portable/GCC/ARM_CM3" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/portable/MemMang" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/source" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/config" -I"D:/stm32cubeide workspace/GameConsole/User/UserDrivers/UserDriversInc" -I"D:/stm32cubeide workspace/GameConsole/User/UserDrivers" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode/UserCodeInc" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-UserCode-2f-UserCodeSrc

clean-User-2f-UserCode-2f-UserCodeSrc:
	-$(RM) ./User/UserCode/UserCodeSrc/ButtonsProcessing.cyclo ./User/UserCode/UserCodeSrc/ButtonsProcessing.d ./User/UserCode/UserCodeSrc/ButtonsProcessing.o ./User/UserCode/UserCodeSrc/ButtonsProcessing.su ./User/UserCode/UserCodeSrc/CommonFreeRTOS.cyclo ./User/UserCode/UserCodeSrc/CommonFreeRTOS.d ./User/UserCode/UserCodeSrc/CommonFreeRTOS.o ./User/UserCode/UserCodeSrc/CommonFreeRTOS.su ./User/UserCode/UserCodeSrc/HooksFreeRTOS.cyclo ./User/UserCode/UserCodeSrc/HooksFreeRTOS.d ./User/UserCode/UserCodeSrc/HooksFreeRTOS.o ./User/UserCode/UserCodeSrc/HooksFreeRTOS.su ./User/UserCode/UserCodeSrc/InterruptsPriorities.cyclo ./User/UserCode/UserCodeSrc/InterruptsPriorities.d ./User/UserCode/UserCodeSrc/InterruptsPriorities.o ./User/UserCode/UserCodeSrc/InterruptsPriorities.su ./User/UserCode/UserCodeSrc/UserTimer.cyclo ./User/UserCode/UserCodeSrc/UserTimer.d ./User/UserCode/UserCodeSrc/UserTimer.o ./User/UserCode/UserCodeSrc/UserTimer.su

.PHONY: clean-User-2f-UserCode-2f-UserCodeSrc

