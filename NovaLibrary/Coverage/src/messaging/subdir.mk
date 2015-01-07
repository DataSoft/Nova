################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/messaging/Message.cpp \
../src/messaging/MessageManager.cpp 

OBJS += \
./src/messaging/Message.o \
./src/messaging/MessageManager.o 

CPP_DEPS += \
./src/messaging/Message.d \
./src/messaging/MessageManager.d 


# Each subdirectory must supply rules for building sources it contributes
src/messaging/%.o: ../src/messaging/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O0 -g3 -Wall -c -fmessage-length=0  -pthread -std=c++0x -fPIC -fprofile-arcs -ftest-coverage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


