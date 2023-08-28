################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/tree_mio.cpp 

OBJS += \
./src/tree_mio.o 

CPP_DEPS += \
./src/tree_mio.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -DNDEBUG -DIL_STD -I/opt/ibm/ILOG/CPLEX_Studio201/cplex/include -I/opt/ibm/ILOG/CPLEX_Studio201/concert/include -O3 -Wall -c -fmessage-length=0 -fno-strict-aliasing -m64 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


