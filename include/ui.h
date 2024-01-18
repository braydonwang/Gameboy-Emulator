#ifndef __UI_H__
#define __UI_H__

#include <common.h>

static const int SCREEN_WIDTH = 1024;
static const int SCREEN_HEIGHT = 768;

void ui_init();
void ui_handle_events();
void ui_update();

#endif /* __UI_H__ */
