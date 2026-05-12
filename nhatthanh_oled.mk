################################################################################
#
# nhatthanh_oled
#
################################################################################

NHATTHANH_OLED_VERSION = 1.0
# Trỏ trực tiếp vào thư mục hiện tại của package để tránh lỗi đường dẫn tuyệt đối
NHATTHANH_OLED_SITE = $(NHATTHANH_OLED_PKGDIR)
NHATTHANH_OLED_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
