#include "ui_common.h"
#include "bsp_config.h"
#include <stdio.h>

#define HEADER_BG_COLOR lv_color_hex(0x00B0FF)
#define HEADER_TEXT_COLOR lv_color_white()
#define HEADER_PADDING 3

lv_obj_t *create_screen_header(lv_obj_t *parent, const char *text)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, EXAMPLE_LCD_H_RES, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(header, HEADER_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *label = lv_label_create(header);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, HEADER_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_center(label);
    lv_obj_set_style_pad_top(header, HEADER_PADDING, 0);
    lv_obj_set_style_pad_bottom(header, HEADER_PADDING, 0);

    return header;
}

void position_content_below_header(lv_obj_t *content, lv_obj_t *header, int margin)
{
    lv_coord_t header_h = lv_obj_get_height(header);
    lv_obj_set_pos(content, 0, header_h + margin);
    lv_obj_set_width(content, EXAMPLE_LCD_H_RES);
}

lv_obj_t *create_content_container(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(cont, CONTENT_ROW_PAD, 0);
    lv_obj_set_width(cont, lv_pct(100));
    return cont;
}