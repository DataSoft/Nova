################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Control.cpp \
../src/Main.cpp \
../src/Novad.cpp \
../src/ProtocolHandler.cpp \
../src/Threads.cpp 

OBJS += \
./src/Control.o \
./src/Main.o \
./src/Novad.o \
./src/ProtocolHandler.o \
./src/Threads.o 

CPP_DEPS += \
./src/Control.d \
./src/Main.d \
./src/Novad.d \
./src/ProtocolHandler.d \
./src/Threads.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


