NHATTHANH_BTL_VERSION = 1.0
NHATTHANH_BTL_SITE = package/nhatthanh_BTL
NHATTHANH_BTL_SITE_METHOD = local

# Build ngay tai thu muc go'c cua package
define NHATTHANH_BTL_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

# Copy file thuc thi vao he thong file cua board
define NHATTHANH_BTL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/nhatthanh_btl $(TARGET_DIR)/usr/bin/nhatthanh_btl
endef

# COPY FILE TU KHOI DONG VAO HE THONG (DOAN MOI THEM)
define NHATTHANH_BTL_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 package/nhatthanh_BTL/S99nhatthanh $(TARGET_DIR)/etc/init.d/S99nhatthanh
endef

$(eval $(generic-package))
