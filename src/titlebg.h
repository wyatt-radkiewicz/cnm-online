#ifndef _titlebg_h_
#define _titlebg_h_

void titlebg_init(void);
void titlebg_cleanup(void);
void titlebg_update(void);
void titlebg_draw(void(*mid_callback)(void));
void titlebg_set_card_movement(int target, int delay, int stagger);

#endif

