################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ClassificationAggregator.cpp \
../src/ClassificationEngine.cpp \
../src/ClassificationEngineFactory.cpp \
../src/Config.cpp \
../src/Database.cpp \
../src/DatabaseQueue.cpp \
../src/Doppelganger.cpp \
../src/Evidence.cpp \
../src/EvidenceAccumulator.cpp \
../src/EvidenceTable.cpp \
../src/FilePacketCapture.cpp \
../src/HaystackControl.cpp \
../src/InterfacePacketCapture.cpp \
../src/KnnClassification.cpp \
../src/Logger.cpp \
../src/MessageManager.cpp \
../src/NovaUtil.cpp \
../src/PacketCapture.cpp \
../src/Point.cpp \
../src/Proxy.cpp \
../src/QuasarDatabase.cpp \
../src/ScriptAlertClassification.cpp \
../src/Suspect.cpp \
../src/ThresholdTriggerClassification.cpp \
../src/UnauthorizedMACClassification.cpp \
../src/UnauthorizedSuspectsClassification.cpp \
../src/WhitelistConfiguration.cpp 

OBJS += \
./src/ClassificationAggregator.o \
./src/ClassificationEngine.o \
./src/ClassificationEngineFactory.o \
./src/Config.o \
./src/Database.o \
./src/DatabaseQueue.o \
./src/Doppelganger.o \
./src/Evidence.o \
./src/EvidenceAccumulator.o \
./src/EvidenceTable.o \
./src/FilePacketCapture.o \
./src/HaystackControl.o \
./src/InterfacePacketCapture.o \
./src/KnnClassification.o \
./src/Logger.o \
./src/MessageManager.o \
./src/NovaUtil.o \
./src/PacketCapture.o \
./src/Point.o \
./src/Proxy.o \
./src/QuasarDatabase.o \
./src/ScriptAlertClassification.o \
./src/Suspect.o \
./src/ThresholdTriggerClassification.o \
./src/UnauthorizedMACClassification.o \
./src/UnauthorizedSuspectsClassification.o \
./src/WhitelistConfiguration.o 

CPP_DEPS += \
./src/ClassificationAggregator.d \
./src/ClassificationEngine.d \
./src/ClassificationEngineFactory.d \
./src/Config.d \
./src/Database.d \
./src/DatabaseQueue.d \
./src/Doppelganger.d \
./src/Evidence.d \
./src/EvidenceAccumulator.d \
./src/EvidenceTable.d \
./src/FilePacketCapture.d \
./src/HaystackControl.d \
./src/InterfacePacketCapture.d \
./src/KnnClassification.d \
./src/Logger.d \
./src/MessageManager.d \
./src/NovaUtil.d \
./src/PacketCapture.d \
./src/Point.d \
./src/Proxy.d \
./src/QuasarDatabase.d \
./src/ScriptAlertClassification.d \
./src/Suspect.d \
./src/ThresholdTriggerClassification.d \
./src/UnauthorizedMACClassification.d \
./src/UnauthorizedSuspectsClassification.d \
./src/WhitelistConfiguration.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O0 -g3 -Wall -c -fmessage-length=0  -pthread -std=c++0x -fPIC -fprofile-arcs -ftest-coverage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


