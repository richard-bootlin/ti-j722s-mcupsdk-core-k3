lpm_s2r.h is generated with:
make DEVICE="${SOC}" PROFILE="$profile" \
	CGT_TI_ARM_CLANG_PATH="${TOOLCHAIN_PATH_R5}" \
	CGT_TI_C7000_PATH="${CGT_TI_C7000_PATH}" \
	SYSCFG_PATH="${SYSCFG_PATH}" \
	-j20 \
	-f "makefile.${SOC}" s2r_wkup-r5f.ti-arm-clang

	${ARM_TOOLCHAIN}ld  -Bstatic -T ./source/drivers/device_manager/s2r/s2r.lds \
		./source/drivers/device_manager/s2r/obj/${SOC}/ti-arm-clang/$profile/wkup-r5f/s2r/lpm_s2r.obj -o lpm_s2r.elf
	${ARM_TOOLCHAIN}objcopy -S -O binary lpm_s2r.elf lpm_s2r.bin
	python3 ./tools/bin2c/bin2c.py lpm_s2r.bin ./source/drivers/device_manager/s2r/lpm_s2r.h lpm_sram_s2r 2026

