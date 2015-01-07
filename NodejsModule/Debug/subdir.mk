################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../CustomizeTraining.cpp \
../HoneydAutoConfigBinding.cpp \
../HoneydConfigBinding.cpp \
../HoneydProfileBinding.cpp \
../HoneydTypesJs.cpp \
../LoggerBinding.cpp \
../NovaConfig.cpp \
../NovaConfigBinding.cpp \
../NovaNode.cpp \
../OsPersonalityDbBinding.cpp \
../SuspectJs.cpp \
../TrainingDumpBinding.cpp \
../VendorMacDbBinding.cpp \
../WhitelistConfigurationBinding.cpp 

OBJS += \
./CustomizeTraining.o \
./HoneydAutoConfigBinding.o \
./HoneydConfigBinding.o \
./HoneydProfileBinding.o \
./HoneydTypesJs.o \
./LoggerBinding.o \
./NovaConfig.o \
./NovaConfigBinding.o \
./NovaNode.o \
./OsPersonalityDbBinding.o \
./SuspectJs.o \
./TrainingDumpBinding.o \
./VendorMacDbBinding.o \
./WhitelistConfigurationBinding.o 

CPP_DEPS += \
./CustomizeTraining.d \
./HoneydAutoConfigBinding.d \
./HoneydConfigBinding.d \
./HoneydProfileBinding.d \
./HoneydTypesJs.d \
./LoggerBinding.d \
./NovaConfig.d \
./NovaConfigBinding.d \
./NovaNode.d \
./OsPersonalityDbBinding.d \
./SuspectJs.d \
./TrainingDumpBinding.d \
./VendorMacDbBinding.d \
./WhitelistConfigurationBinding.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


