GCC_VARIANT:=final

include ../common.mk

GCC_CONFIGURE += \
	--with-headers=$(TOOLCHAIN_DIR)/include \
	--disable-libsanitizer \
	--enable-languages=$(TARGET_LANGUAGES) \
	--enable-shared \
	--enable-threads \
	--with-arch=mips32r2 \
	--with-mtune=1004kc \
	--with-slibdir=$(TOOLCHAIN_DIR)/lib

ifneq ($(CONFIG_GCC_VERSION_4_5)$(CONFIG_GCC_VERSION_4_6),)
  GCC_CONFIGURE += \
	--enable-lto \
	--with-libelf=$(TOPDIR)/staging_dir/host
endif

ifneq ($(CONFIG_SJLJ_EXCEPTIONS),)
  GCC_CONFIGURE += \
	--enable-sjlj-exceptions
endif

define FixToolchainGcc5
	# Fixup links if using eglibc
	$(if $(CONFIG_USE_EGLIBC), $(if $(CONFIG_GCC_VERSION_5), (\
      mkdir -p $(GCC_BUILD_DIR)/gotools; \
      ln -sf $(GCC_BUILD_DIR)/../$(CONFIG_LIBC)-dev/lib/crti.o $(GCC_BUILD_DIR)/gotools/crti.o; \
      ln -sf $(GCC_BUILD_DIR)/../$(CONFIG_LIBC)-dev/lib/crt1.o $(GCC_BUILD_DIR)/gotools/crt1.o; \
      ln -sf $(GCC_BUILD_DIR)/../$(CONFIG_LIBC)-dev/lib/crtn.o $(GCC_BUILD_DIR)/gotools/crtn.o; \
    )))
endef

define CleanupToolchain
	$(INSTALL_DIR) $(TOOLCHAIN_DIR)/$(REAL_GNU_TARGET_NAME)
	# Important!  Required for limits.h to be fixed.
	rm -rf $(TOOLCHAIN_DIR)/$(REAL_GNU_TARGET_NAME)/sys-include
	ln -sf ../include $(TOOLCHAIN_DIR)/$(REAL_GNU_TARGET_NAME)/sys-include
	rm -rf $(TOOLCHAIN_DIR)/$(REAL_GNU_TARGET_NAME)/lib
	ln -sf ../lib $(TOOLCHAIN_DIR)/$(REAL_GNU_TARGET_NAME)/lib
	$(if $(CONFIG_ARCH_64BIT),ln -sf ../lib64 $(TOOLCHAIN_DIR)/$(REAL_GNU_TARGET_NAME)/lib64)
endef

define Host/Configure
	$(CleanupToolchain)
	mkdir -p $(GCC_BUILD_DIR)
	(cd $(GCC_BUILD_DIR) && rm -f config.cache; \
		$(GCC_CONFIGURE) \
	);
endef

define Host/Compile
    $(FixToolchainGcc5)
	+$(GCC_MAKE) $(HOST_JOBS) -C $(GCC_BUILD_DIR) all
endef

define SetupExtraArch
	for app in $(TOOLCHAIN_DIR)/bin/$(OPTIMIZE_FOR_CPU)*-{gcc,gcc-*,g++}; do \
		[ -e $$$$app ] || continue; \
		old_base=$$$$(basename $$$$app); \
		new_base=$(call qstrip,$(CONFIG_EXTRA_TARGET_ARCH_NAME))-$$$${old_base##$(OPTIMIZE_FOR_CPU)-}; \
		sed -e "s/@CC_BASE@/$$$$old_base/" \
			-e 's/@EXTRA_ARCH_OPTS@/$(call qstrip,$(CONFIG_EXTRA_TARGET_ARCH_OPTS))/' \
			 ../files/alternate-arch-cc.in > \
			 $(TOOLCHAIN_DIR)/bin/$$$$new_base; \
		chmod a+x $(TOOLCHAIN_DIR)/bin/$$$$new_base; \
	done
endef

define Host/Install
	$(CleanupToolchain)
	$(_SINGLE)$(GCC_MAKE) -C $(GCC_BUILD_DIR) install
	# Set up the symlinks to enable lying about target name.
	set -e; \
	(cd $(TOOLCHAIN_DIR); \
		ln -sf $(REAL_GNU_TARGET_NAME) $(GNU_TARGET_NAME); \
		cd bin; \
		for app in $(REAL_GNU_TARGET_NAME)-* ; do \
			ln -sf $$$${app} \
		   	$(GNU_TARGET_NAME)$$$${app##$(REAL_GNU_TARGET_NAME)}; \
		done; \
	);
	$(if $(CONFIG_EXTRA_TARGET_ARCH),$(call SetupExtraArch))
	$(RM) $(TOOLCHAIN_DIR)/lib/libiberty.a
	$(SCRIPT_DIR)/patch-specs.sh "$(TOOLCHAIN_DIR)"
endef

$(eval $(call HostBuild))

