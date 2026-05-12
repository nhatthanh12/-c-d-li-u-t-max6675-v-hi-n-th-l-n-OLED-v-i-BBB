################################################################################
#
# nhatthanh_max6675
#
################################################################################

NHATTHANH_MAX6675_VERSION = 1.0
NHATTHANH_MAX6675_SITE = $(TOPDIR)/package/nhatthanh_max6675
NHATTHANH_MAX6675_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
