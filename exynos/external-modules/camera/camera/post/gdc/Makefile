gdc-objs := pablo-gdc-core.o

obj-$(CONFIG_VIDEO_EXYNOS_CAMERA_POSTPROCESS_GDC)	+= gdc.o

ifeq ($(CONFIG_CAMERA_PP_GDC_VOTF_USE_MFC_API),)
	gdc-m += pablo-gdc-votf-gdc-api.o
else
	gdc-$(CONFIG_CAMERA_PP_GDC_VOTF_USE_MFC_API) += pablo-gdc-votf-mfc-api.o
endif

gdc-$(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)  += pablo-gdc-hdr10p.o
gdc-$(CONFIG_CAMERA_PP_GDC_V1_0_0_OBJ)	+= pablo-hw-api-gdc-v100.o
gdc-$(CONFIG_CAMERA_PP_GDC_V2_0_0_OBJ)	+= pablo-hw-api-gdc-v200.o
gdc-$(CONFIG_CAMERA_PP_GDC_V2_1_0_OBJ)	+= pablo-hw-api-gdc-v210.o
gdc-$(CONFIG_CAMERA_PP_GDC_V2_2_0_OBJ)	+= pablo-hw-api-gdc-v220.o
gdc-$(CONFIG_CAMERA_PP_GDC_V3_0_0_OBJ)	+= pablo-hw-api-gdc-v300.o
gdc-$(CONFIG_CAMERA_PP_GDC_V3_1_0_OBJ)  += pablo-hw-api-gdc-v301.o
gdc-$(CONFIG_CAMERA_PP_GDC_V3_10_0_OBJ)	+= pablo-hw-api-gdc-v310.o
gdc-$(CONFIG_CAMERA_PP_GDC_V4_1_0_OBJ)	+= pablo-hw-api-gdc-v401.o
gdc-$(CONFIG_CAMERA_PP_GDC_V10_0_0_OBJ)	+= pablo-hw-api-gdc-v1000.o
gdc-$(CONFIG_CAMERA_PP_GDC_V10_1_1_OBJ)	+= pablo-hw-api-gdc-v1011.o
gdc-$(CONFIG_CAMERA_PP_GDC_V11_0_0_OBJ) += pablo-hw-api-gdc-v1100.o
gdc-$(CONFIG_CAMERA_PP_GDC_V11_20_0_OBJ) += pablo-hw-api-gdc-v1120.o
gdc-$(CONFIG_CAMERA_PP_GDC_V12_0_0_OBJ) += pablo-hw-api-gdc-v1200.o
gdc-$(CONFIG_CAMERA_PP_GDC_V13_0_0_OBJ) += pablo-hw-api-gdc-v1300.o
gdc-$(CONFIG_CAMERA_PP_GDC_V13_20_0_OBJ) += pablo-hw-api-gdc-v1320.o
