################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.c \
../User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.c 

OBJS += \
./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.o \
./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.o 

C_DEPS += \
./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.d \
./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.d 


# Each subdirectory must supply rules for building sources it contributes
User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/%.o User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/%.su User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/%.cyclo: ../User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/%.c User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/include" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/portable/GCC/ARM_CM3" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/portable/MemMang" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/source" -I"D:/stm32cubeide workspace/GameConsole/FreeRTOS/config" -I"D:/stm32cubeide workspace/GameConsole/User/UserDrivers/UserDriversInc" -I"D:/stm32cubeide workspace/GameConsole/User/UserDrivers" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode/UserCodeInc" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode" -I"D:/stm32cubeide workspace/GameConsole/User/UserCode/UserCodeInc/UserTasksFreeRTOS_Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-UserCode-2f-UserCodeSrc-2f-UserTasksFreeRTOS_Src

clean-User-2f-UserCode-2f-UserCodeSrc-2f-UserTasksFreeRTOS_Src:
	-$(RM) ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.cyclo ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.d ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.o ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/ButtonsProcessing.su ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.cyclo ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.d ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.o ./User/UserCode/UserCodeSrc/UserTasksFreeRTOS_Src/LedsControl.su

.PHONY: clean-User-2f-UserCode-2f-UserCodeSrc-2f-UserTasksFreeRTOS_Src

