# U-boot configuration for the OX810

# U-boot's location when executing from DDR
TEXT_BASE = 0x48d00000

PLL400 ?= 800000000
RPSCLK ?= 25000000

FPGA ?= 0
FPGA_ARM_CLK ?= 25000000

PROBE_MEM_SIZE ?= 0

# Memory size in megabytes if probing is not enabled
MEM_SIZE ?= 64

# Overwrite the mem= Linux bootarg stored in the environment with the amount
# determined from probing or MEM_SIZE above
SET_LINUX_MEM_SIZE ?= 1

# May not be applicable for 820
MEM_ODT ?= 150

USE_SATA ?= 1
USE_SATA_ENV ?= 1
USE_FLASH ?= 0

LINUX_ROOT_RAIDED ?= 1

USE_EXTERNAL_UART ?= 0
INTERNAL_UART ?= 2

# uses leon counted time to update system time on power-up
USE_LEON_TIME_COUNT ?= 1

# This converts Make variables into C preprocessor #defines
PLATFORM_CPPFLAGS += -DNAS_VERSION=$(NAS_VERSION)\
		     -DLINUX_ROOT_RAIDED=$(LINUX_ROOT_RAIDED)\
		     -DMEM_ODT=$(MEM_ODT)\
		     -DPROBE_MEM_SIZE=$(PROBE_MEM_SIZE)\
		     -DMEM_SIZE=$(MEM_SIZE)\
			 -DSET_LINUX_MEM_SIZE=$(SET_LINUX_MEM_SIZE)\
		     -DFPGA=$(FPGA)\
		     -DFPGA_ARM_CLK=$(FPGA_ARM_CLK)\
		     -DINTERNAL_UART=$(INTERNAL_UART)\
		     -DUSE_EXTERNAL_UART=$(USE_EXTERNAL_UART)\
		     -DPLL400=$(PLL400)\
		     -DRPSCLK=$(RPSCLK)\
		     -DUSE_SATA=$(USE_SATA)\
		     -DUSE_SATA_ENV=$(USE_SATA_ENV)\
		     -DUSE_FLASH=$(USE_FLASH)\
		     -DUSE_LEON_TIME_COUNT=$(USE_LEON_TIME_COUNT)
