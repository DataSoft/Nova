################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/HoneydConfiguration/Broadcast.cpp \
../src/HoneydConfiguration/HoneydConfiguration.cpp \
../src/HoneydConfiguration/OsPersonalityDb.cpp \
../src/HoneydConfiguration/Port.cpp \
../src/HoneydConfiguration/PortSet.cpp \
../src/HoneydConfiguration/Profile.cpp \
../src/HoneydConfiguration/ProfileTree.cpp \
../src/HoneydConfiguration/ScannedHost.cpp \
../src/HoneydConfiguration/ScannedHostTable.cpp \
../src/HoneydConfiguration/VendorMacDb.cpp 

OBJS += \
./src/HoneydConfiguration/Broadcast.o \
./src/HoneydConfiguration/HoneydConfiguration.o \
./src/HoneydConfiguration/OsPersonalityDb.o \
./src/HoneydConfiguration/Port.o \
./src/HoneydConfiguration/PortSet.o \
./src/HoneydConfiguration/Profile.o \
./src/HoneydConfiguration/ProfileTree.o \
./src/HoneydConfiguration/ScannedHost.o \
./src/HoneydConfiguration/ScannedHostTable.o \
./src/HoneydConfiguration/VendorMacDb.o 

CPP_DEPS += \
./src/HoneydConfiguration/Broadcast.d \
./src/HoneydConfiguration/HoneydConfiguration.d \
./src/HoneydConfiguration/OsPersonalityDb.d \
./src/HoneydConfiguration/Port.d \
./src/HoneydConfiguration/PortSet.d \
./src/HoneydConfiguration/Profile.d \
./src/HoneydConfiguration/ProfileTree.d \
./src/HoneydConfiguration/ScannedHost.d \
./src/HoneydConfiguration/ScannedHostTable.d \
./src/HoneydConfiguration/VendorMacDb.d 


# Each subdirectory must supply rules for building sources it contributes
src/HoneydConfiguration/%.o: ../src/HoneydConfiguration/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O3 -Wall -c -fmessage-length=0  -pthread -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


