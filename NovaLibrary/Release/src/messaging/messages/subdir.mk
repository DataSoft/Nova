################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/messaging/messages/ControlMessage.cpp \
../src/messaging/messages/ErrorMessage.cpp \
../src/messaging/messages/Message.cpp \
../src/messaging/messages/RequestMessage.cpp \
../src/messaging/messages/UpdateMessage.cpp 

OBJS += \
./src/messaging/messages/ControlMessage.o \
./src/messaging/messages/ErrorMessage.o \
./src/messaging/messages/Message.o \
./src/messaging/messages/RequestMessage.o \
./src/messaging/messages/UpdateMessage.o 

CPP_DEPS += \
./src/messaging/messages/ControlMessage.d \
./src/messaging/messages/ErrorMessage.d \
./src/messaging/messages/Message.d \
./src/messaging/messages/RequestMessage.d \
./src/messaging/messages/UpdateMessage.d 


# Each subdirectory must supply rules for building sources it contributes
src/messaging/messages/%.o: ../src/messaging/messages/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O3 -Wall -c -fmessage-length=0  -pthread -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


