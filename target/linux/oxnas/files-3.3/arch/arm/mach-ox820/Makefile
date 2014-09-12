#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y := pci.o rps-irq.o rps-timer.o localtimer.o 
obj-$(CONFIG_ARCH_OX820) += ox820.o
obj-$(CONFIG_SMP) += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
