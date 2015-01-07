################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/protobuf/marshalled_classes.pb.cc 

OBJS += \
./src/protobuf/marshalled_classes.pb.o 

CC_DEPS += \
./src/protobuf/marshalled_classes.pb.d 


# Each subdirectory must supply rules for building sources it contributes
src/protobuf/%.o: ../src/protobuf/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O0 -g3 -Wall -c -fmessage-length=0  -pthread -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


