#
# Copyright (C) 2013-2014 www.hiwifi.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define KernelPackage/mmc-mtk-mc-host
  SUBMENU:=$(OTHER_MENU)
  TITLE:=MT7620a MMC host controller driver
  DEPENDS:=@TARGET_ralink_mt7620||TARGET_ralink_mt7620a_custom
  KCONFIG:= \
  CONFIG_MTK_MMC=y \
  CONFIG_MTK_AEE_KDUMP=n
  FILES:=$(LINUX_DIR)/drivers/mmc/host/mtk-mmc/mtk_sd.ko
  AUTOLOAD:=$(call AutoLoad,92,mtk_sd,1)
  $(call AddDepends/mmc)
endef

define KernelPackage/mmc-mtk-mc-host/description
  This driver provides MTK MMC host Controller.
endef

$(eval $(call KernelPackage,mmc-mtk-mc-host))

define KernelPackage/rt2860v2_ap
  SUBMENU:=Wireless Drivers
  TITLE:=Ralink Drivers for RT2860 cards (RT2860 PCI)
  DEPENDS:=@TARGET_ralink_mt7620||TARGET_ralink_mt7620a_custom
  FILES:=$(LINUX_DIR)/drivers/net/wireless/rt2860v2_ap/rt2860v2_ap.ko
  AUTOLOAD:=$(call AutoLoad,27,rt2860v2_ap,1)
endef

define KernelPackage/rt2860v2_ap/description
  Ralink Drivers for RT2860 cards (RT2860 PCI).
endef

$(eval $(call KernelPackage,rt2860v2_ap))

define KernelPackage/rt2860v2_normol_ap
  SUBMENU:=Wireless Drivers
  TITLE:=Ralink Drivers for RT2860 cards (RT2860 PCI, INTERNEL PA and LNA)
  DEPENDS:=@TARGET_ralink_mt7620a_custom
  FILES:=$(LINUX_DIR)/drivers/net/wireless/rt2860v2_ap/rt2860v2_ap.ko
  KCONFIG:= \
  CONFIG_INTERNAL_PA_INTERNAL_LNA=y \
  CONFIG_INTERNAL_PA_EXTERNAL_LNA=n \
  CONFIG_EXTERNAL_PA_EXTERNAL_LNA=n
  AUTOLOAD:=$(call AutoLoad,27,rt2860v2_ap,1)
endef

define KernelPackage/rt2860v2_normol_ap/description
  Ralink Drivers for RT2860 cards (RT2860 PCI).
endef

$(eval $(call KernelPackage,rt2860v2_normol_ap))

define KernelPackage/mt7610_ap
  SUBMENU:=Wireless Drivers
  TITLE:=Ralink Drivers for MT7610 cards (MT7610 PCI)
  DEPENDS:=@TARGET_ralink_mt7620||TARGET_ralink_mt7620a_custom
  FILES:=$(LINUX_DIR)/drivers/net/wireless/MT7610_ap/MT7610_ap.ko
  AUTOLOAD:=$(call AutoLoad,29,MT7610_ap,1)
endef

define KernelPackage/mt7610_ap/description
  Ralink Drivers for MT7610 cards (MT7610 PCI).
endef

$(eval $(call KernelPackage,mt7610_ap))

define KernelPackage/mt7610_normol_ap
  SUBMENU:=Wireless Drivers
  TITLE:=Ralink Drivers for MT7610 cards (MT7610 PCI, INTERNEL PA and LNA)
  DEPENDS:=@TARGET_ralink_mt7620a_custom
  FILES:=$(LINUX_DIR)/drivers/net/wireless/MT7610_ap/MT7610_ap.ko
  KCONFIG:= \
  CONFIG_INTERNAL_PA_INTERNAL_LNA=y \
  CONFIG_INTERNAL_PA_EXTERNAL_LNA=n \
  CONFIG_EXTERNAL_PA_EXTERNAL_LNA=n
  AUTOLOAD:=$(call AutoLoad,29,MT7610_ap,1)
endef

define KernelPackage/mt7610_normol_ap/description
  Ralink Drivers for MT7610 cards (MT7610 PCI).
endef

$(eval $(call KernelPackage,mt7610_normol_ap))

define KernelPackage/mt7612e_ap
  SUBMENU:=Wireless Drivers
  TITLE:=Ralink Drivers for MT7612e cards (MT7612e PCI)
  DEPENDS:=@TARGET_ralink_mt7620||TARGET_ralink_mt7620a_custom
  KCONFIG:= \
  CONFIG_SECOND_IF_MT7612E=y \
  CONFIG_RLT_WIFI \
  CONFIG_FIRST_IF_EEPROM_PROM=n \
  CONFIG_FIRST_IF_EEPROM_EFUSE=n \
  CONFIG_FIRST_IF_EEPROM_FLASH=y \
  CONFIG_RT_FIRST_CARD_EEPROM="flash" \
  CONFIG_SECOND_IF_EEPROM_PROM=n \
  CONFIG_SECOND_IF_EEPROM_EFUSE=n \
  CONFIG_SECOND_IF_EEPROM_FLASH=y \
  CONFIG_RT_SECOND_CARD_EEPROM="flash" \
  CONFIG_MULTI_INF_SUPPORT=y \
  CONFIG_WIFI_BASIC_FUNC=y \
  CONFIG_WSC_INCLUDED=y \
  CONFIG_WSC_V2_SUPPORT=y \
  CONFIG_WSC_NFC_SUPPORT=n \
  CONFIG_DOT11N_DRAFT3=y \
  CONFIG_DOT11_VHT_AC=y \
  CONFIG_DOT11W_PMF_SUPPORT=n \
  CONFIG_TXBF_SUPPORT=n \
  CONFIG_LLTD_SUPPORT=n \
  CONFIG_QOS_DLS_SUPPORT=n \
  CONFIG_WAPI_SUPPORT=n \
  CONFIG_CARRIER_DETECTION_SUPPORT=n \
  CONFIG_ED_MONITOR_SUPPORT=n \
  CONFIG_IGMP_SNOOP_SUPPORT=y \
  CONFIG_BLOCK_NET_IF=n \
  CONFIG_RATE_ADAPTION=y \
  CONFIG_NEW_RATE_ADAPT_SUPPORT=y \
  CONFIG_AGS_SUPPORT=n \
  CONFIG_IDS_SUPPORT=n \
  CONFIG_WIFI_WORK_QUEUE=n \
  CONFIG_WIFI_SKB_RECYCLE=n \
  CONFIG_RTMP_FLASH_SUPPORT=y \
  CONFIG_LED_CONTROL_SUPPORT=n \
  CONFIG_SINGLE_SKU_V2=n \
  CONFIG_ATE_SUPPORT=y \
  CONFIG_RT2860V2_AP_32B_DESC=n \
  CONFIG_MEMORY_OPTIMIZATION=n \
  CONFIG_HOTSPOT=n \
  CONFIG_CO_CLOCK_SUPPORT=n \
  CONFIG_FIRST_CARD_EXTERNAL_PA=n \
  CONFIG_FIRST_CARD_EXTERNAL_LNA=n \
  CONFIG_SECOND_CARD_EXTERNAL_PA=n \
  CONFIG_SECOND_CARD_EXTERNAL_LNA=n \
  CONFIG_RLT_MAC=y \
  CONFIG_RTMP_MAC=y \
  CONFIG_RTMP_PCI_SUPPORT=y \
  CONFIG_WIFI_MODE_AP=y \
  CONFIG_WIFI_MODE_STA=n \
  CONFIG_WIFI_MODE_BOTH=n \
  CONFIG_RLT_AP_SUPPORT \
  CONFIG_WDS_SUPPORT=n \
  CONFIG_MBSS_SUPPORT=y \
  CONFIG_ENHANCE_NEW_MBSSID_MODE=y \
  CONFIG_APCLI_SUPPORT=y \
  CONFIG_APCLI_CERT_SUPPORT=n \
  CONFIG_MAC_REPEATER_SUPPORT=n \
  CONFIG_DFS_SUPPORT=n \
  CONFIG_NINTENDO_AP=n \
  CONFIG_COC_SUPPORT=n \
  CONFIG_DELAYED_TCP_ACK_SUPPORT=n \
  CONFIG_RALINK_RT28XX=n \
  CONFIG_RALINK_RT3092=n \
  CONFIG_RALINK_RT3572=n \
  CONFIG_RALINK_RT5392=n \
  CONFIG_RALINK_RT5572=n \
  CONFIG_RALINK_RT5592=n \
  CONFIG_RALINK_RT6352=n \
  CONFIG_RALINK_MT7610E=n \
  CONFIG_RALINK_MT7610U=n \
  CONFIG_RALINK_RT8592=n \
  CONFIG_RALINK_MT7612E=y \
  CONFIG_RALINK_MT7612U=n
  FILES:=$(LINUX_DIR)/drivers/net/wireless/rlt_wifi_ap/rlt_wifi.ko
  AUTOLOAD:=$(call AutoLoad,29,rlt_wifi,1)
endef

define KernelPackage/mt7612e_ap/description
  Ralink Drivers for MT7612e cards (MT7612e PCI).
endef

$(eval $(call KernelPackage,mt7612e_ap))

define KernelPackage/mtk-hw-nat
  SUBMENU:=$(OTHER_MENU)
  TITLE:=MT7620a Hardware nat driver
  DEPENDS:=@TARGET_ralink_mt7620||TARGET_ralink_mt7620a_custom
  FILES:=$(LINUX_DIR)/net/nat/hw_nat/hw_nat.ko
endef

define KernelPackage/mtk-hw-nat/description
  This driver support MT7620A hardware nat function.
endef

$(eval $(call KernelPackage,mtk-hw-nat))
