#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "pc/lua/utils/smlua_misc_utils.h"

static char sInfo[512] = { 0 };

void djui_panel_info_create(struct DjuiBase *caller) {
    struct DjuiThreePanel *panel = djui_panel_menu_create(DLANG(INFORMATION, INFORMATION_TITLE), false);
    struct DjuiBase *body = djui_three_panel_get_body(panel);
    {
        snprintf(sInfo, 512, "\
感谢游玩蘑菇云译社的汉化版本！更新&聊天加QQ群聊981281124\n(资源只允许在官方群聊下载，如果你是通过别的渠道下载的，请加入群聊向我们申诉)\n特别鸣谢：梅塔的长名字、小花jacob、SGF3、xXram2dieXx、狗哥又玩又爱玩、小扬awa、超级屑的屑蓝猫、一只普通的刺猬、Toad114514、216、54wxw、Torbi、大聪明H、xiao-Link、雪狼SnowWolf、脚滑的冰块、rew_lezi64以及所有Mod汉化者\n当前汉化版本:1.0.1");

        struct DjuiText* text = djui_text_create(body, sInfo);
        djui_base_set_location(&text->base, 0, 0);
        djui_base_set_size(&text->base, (DJUI_DEFAULT_PANEL_WIDTH * (configDjuiThemeCenter ? DJUI_THEME_CENTERED_WIDTH : 1)) - 64, 400);
        djui_base_set_color(&text->base, 220, 220, 220, 255);
        djui_text_set_drop_shadow(text, 64, 64, 64, 100);
        djui_text_set_alignment(text, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
