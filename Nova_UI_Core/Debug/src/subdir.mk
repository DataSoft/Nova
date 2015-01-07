################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Connection.cpp \
../src/NovadControl.cpp \
../src/StatusQueries.cpp \
../src/TrainingData.cpp \
../src/TrainingDump.cpp 

OBJS += \
./src/Connection.o \
./src/NovadControl.o \
./src/StatusQueries.o \
./src/TrainingData.o \
./src/TrainingDump.o 

CPP_DEPS += \
./src/Connection.d \
./src/NovadControl.d \
./src/StatusQueries.d \
./src/TrainingData.d \
./src/TrainingDump.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


