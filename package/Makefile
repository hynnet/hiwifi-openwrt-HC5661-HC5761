#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

curdir:=package

-include $(TMP_DIR)/.packagedeps
$(curdir)/builddirs:=$(sort $(package-) $(package-y) $(package-m))
$(curdir)/builddirs-install:=.
ifeq ($(SDK),1)
else
  $(curdir)/builddirs-default:=. $(sort $(package-y) $(package-m))
  $(curdir)/builddirs-prereq:=. $(sort $(prereq-y) $(prereq-m))
endif
ifneq ($(IGNORE_ERRORS),)
  $(curdir)/builddirs-ignore-compile:= $(if $(filter n m y, $(IGNORE_ERRORS)),$(foreach m,$(IGNORE_ERRORS),$(package-$(subst n,,$(m)))),$(package-m) $(package-))
endif

ifdef CONFIG_USE_MKLIBS
  define mklibs
	rm -rf $(TMP_DIR)/mklibs-progs $(TMP_DIR)/mklibs-out
	# first find all programs and add them to the mklibs list
	find $(STAGING_DIR_ROOT) -type f -perm +100 -exec \
		file -r -N -F '' {} + | \
		awk ' /executable.*dynamically/ { print $$1 }' > $(TMP_DIR)/mklibs-progs
	# find all loadable objects that are not regular libraries and add them to the list as well
	find $(STAGING_DIR_ROOT) -type f -name \*.so\* -exec \
		file -r -N -F '' {} + | \
		awk ' /shared object/ { print $$1 }' >> $(TMP_DIR)/mklibs-progs
	mkdir -p $(TMP_DIR)/mklibs-out
	$(STAGING_DIR_HOST)/bin/mklibs -D \
		-d $(TMP_DIR)/mklibs-out \
		--sysroot $(STAGING_DIR_ROOT) \
		-L /lib \
		-L /usr/lib \
		-L /usr/lib/ebtables \
		--ldlib $(patsubst $(STAGING_DIR_ROOT)/%,/%,$(firstword $(wildcard \
			$(foreach name,ld-uClibc.so.* ld-linux.so.* ld-*.so, \
			  $(STAGING_DIR_ROOT)/lib/$(name) \
			)))) \
		--target $(REAL_GNU_TARGET_NAME) \
		`cat $(TMP_DIR)/mklibs-progs` 2>&1
	$(RSTRIP) $(TMP_DIR)/mklibs-out
	for lib in `ls $(TMP_DIR)/mklibs-out/*.so.* 2>/dev/null`; do \
		LIB="$${lib##*/}"; \
		DEST="`ls "$(TARGET_DIR)/lib/$$LIB" "$(TARGET_DIR)/usr/lib/$$LIB" 2>/dev/null`"; \
		[ -n "$$DEST" ] || continue; \
		echo "Copying stripped library $$lib to $$DEST"; \
		cp "$$lib" "$$DEST" || exit 1; \
	done
  endef
endif

# where to build (and put) .ipk packages
OPKG:= \
  IPKG_TMP=$(TMP_DIR)/ipkg \
  IPKG_INSTROOT=$(TARGET_DIR) \
  IPKG_CONF_DIR=$(STAGING_DIR)/etc \
  IPKG_OFFLINE_ROOT=$(TARGET_DIR) \
  $(STAGING_DIR_HOST)/bin/opkg \
	--offline-root $(TARGET_DIR) \
	--force-depends \
	--force-overwrite \
	--force-postinstall \
	--force-maintainer \
	--add-dest root:/ \
	--add-arch all:100 \
	--add-arch $(if $(ARCH_PACKAGES),$(ARCH_PACKAGES),$(BOARD)):200

# opkg install debug package
DEBUG_OPKG:= \
  IPKG_TMP=$(TMP_DIR)/ipkg \
  IPKG_INSTROOT=$(DEBUG_TARGET_DIR) \
  IPKG_CONF_DIR=$(STAGING_DIR)/etc \
  IPKG_OFFLINE_ROOT=$(DEBUG_TARGET_DIR) \
  $(STAGING_DIR_HOST)/bin/opkg \
	--offline-root $(DEBUG_TARGET_DIR) \
	--force-depends \
	--force-overwrite \
	--force-postinstall \
	--force-maintainer \
	--add-dest root:/ \
	--add-arch all:100 \
	--add-arch $(if $(ARCH_PACKAGES),$(ARCH_PACKAGES),$(BOARD)):200

PACKAGE_INSTALL:=$(sort $(foreach pkg,$(package-y),$(lastword $(subst /,$(space),$(pkg)))))
PACKAGE_INSTALL_FILES:=$(patsubst %,$(PKG_INFO_DIR)/%.install,$(PACKAGE_INSTALL))

$(curdir)/cleanup: $(TMP_DIR)/.build
	rm -rf $(STAGING_DIR_ROOT)

debug_install:
	
$(curdir)/install: $(TMP_DIR)/.build debug_install
	- find $(STAGING_DIR_ROOT) -type d | $(XARGS) chmod 0755

	rm -rf $(DEBUG_TARGET_DIR)
	[ -d $(DEBUG_TARGET_DIR)/tmp ] || mkdir -p $(DEBUG_TARGET_DIR)/tmp
	$(DEBUG_OPKG) install `cat $(PACKAGE_INSTALL_FILES) | sed -e 's,^\(.*\)$$,$(DEBUG_PACKAGE_DIR)/\1_*.ipk,'`

	rm -rf $(TARGET_DIR)
	[ -d $(TARGET_DIR)/tmp ] || mkdir -p $(TARGET_DIR)/tmp
	$(OPKG) install `cat $(PACKAGE_INSTALL_FILES) | sed -e 's,^\(.*\)$$,$(PACKAGE_DIR)/\1_*.ipk,'`
	@for pkg in $(PACKAGE_INSTALL); do \
		[ -s $(PKG_INFO_DIR)/$${pkg}.install.flags ] || continue; \
		for flag in `cat $(PKG_INFO_DIR)/$${pkg}.install.flags`; do \
			$(OPKG) flag $$flag `cat $(PKG_INFO_DIR)/$${pkg}.install`; \
		done; \
	done || true

	#XXX:Make package preconfig
	@-$(MAKE) package/preconfig
	@if [ -d $(TOPDIR)/files ]; then \
		( cd $(TOPDIR)/files; find -type f ) | \
			( cd $(TARGET_DIR); while :; do \
				read FILE; \
				[ -z "$$FILE" ] && break; \
				[ -L "$$FILE" ] || continue; \
				echo "Removing symlink $(TARGET_DIR)/$$FILE"; \
				rm -f "$$FILE"; \
			done; ); \
		$(CP) $(TOPDIR)/files/. $(TARGET_DIR); \
		( cd $(TOPDIR)/files; find -type f ) | \
			( cd $(DEBUG_TARGET_DIR); while :; do \
				read FILE; \
				[ -z "$$FILE" ] && break; \
				[ -L "$$FILE" ] || continue; \
				echo "Removing symlink $(DEBUG_TARGET_DIR)/$$FILE"; \
				rm -f "$$FILE"; \
			done; ); \
		$(CP) $(TOPDIR)/files/. $(DEBUG_TARGET_DIR); \
	fi

	@mkdir -p $(TARGET_DIR)/etc/rc.d
	@( \
		cd $(TARGET_DIR); \
		for script in ./etc/init.d/*; do \
			grep '#!/bin/sh /etc/rc.common' $$script >/dev/null || continue; \
			IPKG_INSTROOT=$(TARGET_DIR) $$(which bash) ./etc/rc.common $$script enable; \
		done || true \
	)
	@-find $(TARGET_DIR) -name CVS   | $(XARGS) rm -rf
	@-find $(TARGET_DIR) -name .svn  | $(XARGS) rm -rf
	@-find $(TARGET_DIR) -name '.#*' | $(XARGS) rm -f
	rm -f $(TARGET_DIR)/usr/lib/opkg/info/*.postinst
	$(if $(CONFIG_CLEAN_IPKG),rm -rf $(TARGET_DIR)/usr/lib/opkg)
	
	@mkdir -p $(DEBUG_TARGET_DIR)/etc/rc.d
	@( \
		cd $(DEBUG_TARGET_DIR); \
		for script in ./etc/init.d/*; do \
			grep '#!/bin/sh /etc/rc.common' $$script >/dev/null || continue; \
			IPKG_INSTROOT=$(DEBUG_TARGET_DIR) $$(which bash) ./etc/rc.common $$script enable; \
		done || true \
	)
	@-find $(DEBUG_TARGET_DIR) -name CVS   | $(XARGS) rm -rf
	@-find $(DEBUG_TARGET_DIR) -name .svn  | $(XARGS) rm -rf
	@-find $(DEBUG_TARGET_DIR) -name '.#*' | $(XARGS) rm -f
	rm -f $(DEBUG_TARGET_DIR)/usr/lib/opkg/info/*.postinst
	$(if $(CONFIG_CLEAN_IPKG),rm -rf $(DEBUG_TARGET_DIR)/usr/lib/opkg)
	#FIXME: Does it need for make Debug rootfs? 
	$(call mklibs)

	# LIKAI
	# add REVISION infomation to /etc/.build
	#echo $(REVISION) $(shell date --rfc-3339=seconds) > $(TOPDIR)/.build
	echo $(REVISION) $(shell date +%y%m%d-%H%M%S) > $(TOPDIR)/.build
	$(CP) $(TOPDIR)/.build $(TARGET_DIR)/etc/.build

	$(CP) $(TOPDIR)/.build $(DEBUG_TARGET_DIR)/etc/.build

	# disable console shell 
	# disable sshd
	# disable telnetd
	# disable failsafe
	if [ "$(R)" == "stable" ]; then \
		> $(TARGET_DIR)/etc/uci-defaults/inittab-console-fixup; \
		rm -f $(TARGET_DIR)/etc/rc.d/S50dropbear; \
		rm -f $(TARGET_DIR)/etc/rc.d/S50telnet; \
		rm -f $(TARGET_DIR)/lib/preinit/99_10_failsafe_login; \
	fi

	if [ "$(R)" == "stable" ]; then \
		> $(DEBUG_TARGET_DIR)/etc/uci-defaults/inittab-console-fixup; \
		rm -f $(DEBUG_TARGET_DIR)/etc/rc.d/S50dropbear; \
		rm -f $(DEBUG_TARGET_DIR)/etc/rc.d/S50telnet; \
		rm -f $(DEBUG_TARGET_DIR)/lib/preinit/99_10_failsafe_login; \
	fi

$(curdir)/index: FORCE
	@(cd $(PACKAGE_DIR); $(SCRIPT_DIR)/ipkg-make-index.sh . 2>&1 > Packages && \
		gzip -9c Packages > Packages.gz \
	)

$(curdir)/preconfig:

$(curdir)/flags-install:= -j1

$(eval $(call stampfile,$(curdir),package,prereq,.config))
$(eval $(call stampfile,$(curdir),package,cleanup,$(TMP_DIR)/.build))
$(eval $(call stampfile,$(curdir),package,compile,$(TMP_DIR)/.build))
$(eval $(call stampfile,$(curdir),package,install,$(TMP_DIR)/.build))

$(eval $(call subdir,$(curdir)))
