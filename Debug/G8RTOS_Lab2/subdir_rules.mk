################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
G8RTOS_Lab2/%.obj: ../G8RTOS_Lab2/%.s $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/ti/ccsv8/ccs_base/arm/include" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/uart" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/BoardSupportPackage/DriverLib" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/BoardSupportPackage/inc" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/BoardSupportPackage/src" --include_path="C:/ti/ccsv8/ccs_base/arm/include/CMSIS" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --advice:power="all" --define=__MSP432P401R__ --define=ccs -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="G8RTOS_Lab2/$(basename $(<F)).d_raw" --obj_directory="G8RTOS_Lab2" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

G8RTOS_Lab2/%.obj: ../G8RTOS_Lab2/%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/ti/ccsv8/ccs_base/arm/include" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/uart" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/BoardSupportPackage/DriverLib" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/BoardSupportPackage/inc" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3/BoardSupportPackage/src" --include_path="C:/ti/ccsv8/ccs_base/arm/include/CMSIS" --include_path="C:/Users/Johnny/codecomposerworkspace/up2/lab3" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --advice:power="all" --define=__MSP432P401R__ --define=ccs -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="G8RTOS_Lab2/$(basename $(<F)).d_raw" --obj_directory="G8RTOS_Lab2" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


