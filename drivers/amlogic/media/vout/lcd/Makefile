define config_status
$(if $(value $1),y,n)
endef

DPI_info       = DPI[$(call config_status,CONFIG_AML_LCD_DPI)]
lvds_info      = LVDS[$(call config_status,CONFIG_AML_LCD_LVDS)]
tcon_info      = Tcon[$(call config_status,CONFIG_AML_LCD_TCON)]
vbyone_info    = VbyOne[$(call config_status,CONFIG_AML_LCD_VBYONE)]
dsi_info       = MIPI-DSI[$(call config_status,CONFIG_AML_LCD_MIPI_DSI)]
connector_info = connector($(DPI_info) $(lvds_info) $(tcon_info) $(vbyone_info) $(dsi_info))

feature_lcd_cus_ctrl = Cus_Ctrl[$(call config_status,CONFIG_AML_LCD_CUSTOM_CTRL)]
feature_lcd_extern = Extern[$(call config_status,CONFIG_AML_LCD_EXTERN)]
feature_ambilight  = ambilight[$(call config_status,CONFIG_AMLOGIC_AMBILIGHT)]
feature_info       = feature($(feature_lcd_cus_ctrl) $(feature_lcd_extern) $(feature_ambilight))

bl         = Backlight[$(call config_status,CONFIG_AML_LCD_BACKLIGHT)]
bl_extern  = Extern[$(call config_status,CONFIG_AML_LCD_BL_EXTERN)]
bl_ldim    = LDIM[$(call config_status,CONFIG_AML_LCD_BL_LDIM)]
bl_mcu     = LDIM-BLMCU[$(call config_status,CONFIG_AMLOGIC_BL_LDIM_BLMCU)]
bl_info    = $(bl)($(bl_extern) $(bl_ldim) $(bl_mcu))

$(info * Compile Amlogic LCD Module With $(connector_info), $(feature_info), $(bl_info) *)

obj-$(CONFIG_AML_LCD) += lcd_vout.o
obj-$(CONFIG_AML_LCD) += lcd_common.o
obj-$(CONFIG_AML_LCD) += lcd_config.o
obj-$(CONFIG_AML_LCD) += lcd_gpio.o
obj-$(CONFIG_AML_LCD) += lcd_debug.o
obj-$(CONFIG_AML_LCD) += lcd_unifykey.o
obj-$(CONFIG_AML_LCD) += lcd_cus_ctrl.o
obj-$(CONFIG_AML_LCD) += lcd_i2c_dev.o
obj-$(CONFIG_AML_LCD) += lcd_memory.o
obj-$(CONFIG_AML_LCD) += lcd_dphy_ctrl.o

obj-$(CONFIG_AML_LCD) += lcd_parser/
obj-$(CONFIG_AML_LCD) += lcd_venc/
obj-$(CONFIG_AML_LCD) += lcd_clk/
obj-$(CONFIG_AML_LCD) += lcd_phy/
obj-$(CONFIG_AML_LCD) += lcd_addons/
obj-$(CONFIG_AML_LCD) += connectors/

obj-$(CONFIG_AML_LCD) += lcd_mode_tv.o
obj-$(CONFIG_AML_LCD) += lcd_mode_tablet.o

obj-$(CONFIG_AML_LCD_EXTERN) += lcd_extern/

obj-$(CONFIG_AML_LCD_BACKLIGHT) += backlight/
