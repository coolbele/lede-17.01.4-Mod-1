include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=fast-classifier
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/fast-classifier/Default
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Support
  DEPENDS:=@IPV6 +kmod-ipt-conntrack +kmod-shortcut-fe +kmod-ipt-conntrack-extra
  TITLE:=Kernel driver for FAST Classifier
  FILES:=$(PKG_BUILD_DIR)/fast-classifier.ko
  KCONFIG:= \
	   CONFIG_NF_CONNTRACK_EVENTS=y \
	   CONFIG_NF_CONNTRACK_CHAIN_EVENTS=y \
	   CONFIG_NF_CONNTRACK_MARK=y \
	   CONFIG_XFRM=y
  PROVIDES:=fast-classifier
endef

define KernelPackage/fast-classifier
  $(call KernelPackage/fast-classifier/Default)
endef

define KernelPackage/fast-classifier/Default/description
FAST Classifier talks to SFE to make decisions about offloading connections
endef

define KernelPackage/fast-classifier/description
$(call KernelPackage/fast-classifier/Default/description)
endef

SFE_MAKE_OPTS := SFE_SUPPORT_IPV6=y

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" \
		$(strip $(SFE_MAKE_OPTS)) \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		CONFIG_FAST_CLASSIFIER=m \
		modules
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/fast-classifier.h $(1)/usr/include/
endef

$(eval $(call KernelPackage,fast-classifier))
