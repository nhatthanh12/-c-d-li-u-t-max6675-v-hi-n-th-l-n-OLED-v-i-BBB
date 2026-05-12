################################################################################
#
# nhatthanh_gpio
#
################################################################################

NHATTHANH_GPIO_VERSION = 1.0
# Dùng biến nội tại của Buildroot, bao gọn gàng và không bao giờ sợ sai đường dẫn
NHATTHANH_GPIO_SITE = $(NHATTHANH_GPIO_PKGDIR)
NHATTHANH_GPIO_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
