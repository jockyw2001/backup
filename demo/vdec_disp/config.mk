include clear-config.mk
COMPILE_TARGET:=bin
SRCS:=  ../mi_disp_tc_common.c vdec_disp.c
LIBS:= mi_sys mi_disp mi_hdmi mi_vdec mi_vpe mi_divp
include add-config.mk
