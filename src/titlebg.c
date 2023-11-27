#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "titlebg.h"
#include "background.h"
#include "blocks.h"
#include "renderer.h"
#include "utility.h"
#include "spawners.h"
#include "game.h"
#include "console.h"
#include "wobj.h"

static int controlling;
static int ai_xdir;
static int ai_ydir;
static int ai_timer;
static int ai_same_speed;
static CNM_RECT title_card;
static CNM_RECT player;
static CNM_RECT wings[4];
//static int cloudsx[2];
static float pos[2];
static float vel[2];

static float _camx = 0.0f, _camy = 0.0f, _camx_coarse = 0.0f, _camy_coarse = 0.0f, _camx_spd = 0.0f, _camy_spd = 0.0f;
static float *_camx_list, *_camy_list;
static int _cam_list_len, _next_cam_target;
#define MAX_TITLE_CARDS 4
static int _title_card_ticker, _title_card_y[MAX_TITLE_CARDS];
static int title_target_y, title_card_stagger;

void titlebg_init(void) {
	controlling = 0;
	ai_xdir = Util_RandInt(0, 1) * 2 - 1;
	ai_ydir = Util_RandInt(0, 1) * 2 - 1;
	ai_timer = Util_RandInt(5, 60);
	ai_same_speed = 0;
	Util_SetRect(&title_card, 0, 0, 304, 96);
	pos[0] = 120.0f;
	pos[1] = 96.0f;
	vel[0] = 10.0f;
	vel[1] = -2.0f;
	Util_SetRect(&player, 384, 96, 32, 32);
	Util_SetRect(wings + 0, 288+48, 0, 48, 48);
	Util_SetRect(wings + 1, 288+48, 48, 48, 48);
	Util_SetRect(wings + 2, 288,  96, 48, 48);
	Util_SetRect(wings + 3, 288+48, 96, 48, 48);

	_cam_list_len = 0;
	_next_cam_target = 1;
	SPAWNER *spawner = NULL;
	spawner = Spawners_Iterate(spawner);
	while (spawner) {
		if (spawner->wobj_type == TT_BOSS_WAYPOINT) _cam_list_len++;
		spawner = Spawners_Iterate(spawner);
	}
	if (!_cam_list_len) _cam_list_len = 2;
	_camx_list = malloc(sizeof(*_camx_list) * _cam_list_len);
	_camy_list = malloc(sizeof(*_camy_list) * _cam_list_len);
	spawner = Spawners_Iterate(NULL);
	while (spawner) {
		if (spawner->wobj_type == TT_BOSS_WAYPOINT)	{
			//Console_Print("asdf");
			_camx_list[spawner->custom_int] = spawner->x;
			_camy_list[spawner->custom_int] = spawner->y;
		}
		spawner = Spawners_Iterate(spawner);
	}
	int start_idx = Util_RandInt(0, _cam_list_len - 2);
	_camx = _camx_list[start_idx];
	_camy = _camy_list[start_idx];
	_next_cam_target = start_idx + 1;
	_camx_coarse = _camx;
	_camy_coarse = _camy;
	_camx_spd = 0.0f;
	_camy_spd = 0.0f;

	//_title_card_ticker = 0;
	//title_target_y = 36;
	//for (int i = 0; i < MAX_TITLE_CARDS; i++) {
	//	_title_card_y[i] = -title_card.h;
	//}
	titlebg_set_card_movement(36, 30, CNM_TRUE);
	for (int i = 0; i < MAX_TITLE_CARDS; i++) {
		_title_card_y[i] = -title_card.h - 2;
	}
}
void titlebg_cleanup(void) {
	//Console_Print("cleanup!");
	if (_camx_list) free(_camx_list);
	_camx_list = NULL;
	if (_camy_list) free(_camy_list);
	_camy_list = NULL;
}
void titlebg_update(void) {
	int last_ydir = ai_ydir;

	controlling--;
	ai_timer--;

	if (controlling <= 0)
	{
		if (ai_timer <= 0)
		{
			ai_timer = Util_RandInt(5, 30);
			if (pos[0] < 140.0f)
				ai_xdir = 1;
			if (pos[0] > 180.0f)
				ai_xdir = -1;

			if (ai_same_speed >= 60)
			{
				if (ai_ydir)
					ai_ydir = 0;
				else
					ai_ydir = Util_RandInt(0, 1) * 2 - 1;
			}
			else
			{
				if (pos[1] < 100.0f)
					ai_ydir = 1;
				else if (pos[1] > 140.0f)
					ai_ydir = -1;
				else
					ai_ydir = 0;
			}
		}

		if (ai_ydir <= -9.0f || ai_ydir >= 1.0f)
			ai_same_speed++;
		else
			ai_same_speed = 0;

		if (ai_xdir == 1 && vel[0] <= 12.0f)
			vel[0] += 2.0f;
		if (ai_xdir == -1 && vel[0] >= -12.0f)
			vel[0] -= 2.0f;
		if (ai_ydir == 1 && vel[0] <= 0.0f)
			vel[1] += 1.0f;
		if (ai_ydir == -1 && vel[0] >= -10.0f)
			vel[1] -= 1.0f;
	}

	vel[0] = vel[0] * 0.9f;
	vel[1] += 0.3f;
	pos[0] += vel[0];
	pos[1] += vel[1];
	if (vel[1] >= 16.0f && controlling)
		vel[1] = 16.0f;
	else if (vel[1] >= 10.0f && !controlling)
		vel[1] = 10.0f;
	if (pos[1] >= 280.0f)
		pos[1] = -33.0f;
	if (pos[1] <= -64.0f)
		pos[1] = 260.0f;
	if (pos[0] >= 360.0f)
		pos[0] = -33.0f;
	if (pos[0] <= -64.0f)
		pos[0] = 350.0f;

	// Camera shit
	if (fabsf(_camx_coarse - _camx_list[_next_cam_target]) < 2.5f &&
		fabsf(_camy_coarse - _camy_list[_next_cam_target]) < 2.5f) {
		_next_cam_target++;
		if (_next_cam_target == _cam_list_len) {
			_camx_coarse = _camx_list[0];
			_camy_coarse = _camy_list[0];
			_camx = _camx_coarse;
			_next_cam_target = 1;
		}
	}
	const float cam_speed = 1.5f, cam_accel = 0.02f;
	if (_camx_coarse < _camx_list[_next_cam_target] - cam_speed * 0.75f) _camx_coarse += cam_speed;
	if (_camx_coarse > _camx_list[_next_cam_target] + cam_speed * 0.75f) _camx_coarse -= cam_speed;
	if (_camy_coarse < _camy_list[_next_cam_target] - cam_speed * 0.75f) _camy_coarse += cam_speed;
	if (_camy_coarse > _camy_list[_next_cam_target] + cam_speed * 0.75f) _camy_coarse -= cam_speed;
	const float deccel_distx = (-1.0f * _camx_spd * _camx_spd)/(-2.0f * cam_accel);
	const float deccel_disty = (-1.0f * _camy_spd * _camy_spd)/(-2.0f * cam_accel);
	if (_camx < _camx_coarse && _camx_spd < cam_speed) _camx_spd += cam_accel;
	if (_camx > _camx_coarse && _camx_spd > -cam_speed) _camx_spd -= cam_accel;
	if (_camy < _camy_coarse && _camy_spd < cam_speed) _camy_spd += cam_accel;
	if (_camy > _camy_coarse && _camy_spd > -cam_speed) _camy_spd -= cam_accel;
	_camx += _camx_spd;
	_camy += _camy_spd;

	if (_camx > _camx_coarse - deccel_distx && _camx < _camx_coarse &&  _camx_spd > 0.0f) _camx_spd -= cam_accel * 2.0f;
	if (_camx < _camx_coarse + deccel_distx && _camx > _camx_coarse &&  _camx_spd < 0.0f) _camx_spd += cam_accel * 2.0f;
	if (_camy > _camy_coarse - deccel_disty && _camy < _camy_coarse &&  _camy_spd > 0.0f) _camy_spd -= cam_accel * 2.0f;
	if (_camy < _camy_coarse + deccel_disty && _camy > _camy_coarse &&  _camy_spd < 0.0f) _camy_spd += cam_accel * 2.0f;
}

static const int bar_colors[] = {
	195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
	205,
	204, 203, 202, 201, 200, 199, 198, 197, 196, 195
};
static const int bar_trans[] = {
	7, 6, 5, 4, 3, 2, 1, 0,
	1, 2, 3, 4, 5, 6, 7
};
static float _colors_pos = 0.0f, _trans_pos = 0.0f;

void titlebg_draw(void(*mid_callback)(void)) {
	if (!Game_GetVar(GAME_VAR_XMAS_MODE)->data.integer)
	{
		Background_Draw(0, (int)_camx, (int)_camy);
	}

	Blocks_DrawBlocks(BLOCKS_BG, (int)_camx, (int)(_camy - 0.5f));
	Blocks_DrawBlocks(BLOCKS_FG, (int)_camx, (int)(_camy - 0.5f));
	Renderer_SaveToEffectsBuffer();
	Blocks_DrawBlocks(BLOCKS_DUMMY_EFFECTS, (int)_camx, (int)(_camy - 0.5f));
	Renderer_SaveToEffectsBuffer();
	Blocks_DrawBlocks(BLOCKS_DUMMY_EFFECTS_EX, (int)_camx, (int)(_camy - 0.5f));

	Util_SetRect(&player, 224 + ((Game_GetFrame() % 2) * 32), 144, 32, 32);
	int hflip = vel[0] < 0.0f;
	if (vel[1] > 3.0f) {
		Renderer_DrawBitmap2((int)pos[0] - 8, (int)pos[1] - 8, wings + 2 + (Game_GetFrame() / 3 % 2), 2, RENDERER_LIGHT, hflip, 0);
	} else {
		Renderer_DrawBitmap2((int)pos[0] - 8, (int)pos[1] - 8, wings + (Game_GetFrame() / 10 % 2), 2, RENDERER_LIGHT, hflip, 0);
	}
	Renderer_DrawBitmap2((int)pos[0], (int)pos[1], &player, 0, RENDERER_LIGHT, hflip, 0.0f);
	
	Background_Draw(1, (int)_camx, (int)_camy);

	if (mid_callback) mid_callback();

	const int frame = Game_GetFrame() / 2;
	float color_spd = sinf((float)frame / 50.0f) / 10.0f;
	float trans_spd = sinf((float)frame / 30.0f) / 50.0f;
	const float color_spd_bar = sinf((float)frame / 40.0f) * 60.0f;
	const float trans_spd_bar = sinf((float)frame / 20.0f) * 30.0f;
	_colors_pos += cosf((float)frame / 50.0f) / 10.0f;
	_trans_pos += cosf((float)frame / 30.0f) / 10.0f;
	float cpos = _colors_pos, tpos = _trans_pos;
	for (int i = 0; i < RENDERER_HEIGHT; i++) {
		cpos += color_spd;
		//color_spd += cosf((float)frame / 40.0f + (float)i / color_spd_bar) / 30.0f;
		tpos += trans_spd;
		//trans_spd += cosf((float)frame / 20.0f + (float)i / trans_spd_bar) / 30.0f;
		int cidx = abs((int)cpos) % (sizeof(bar_colors) / sizeof(*bar_colors));
		int tidx = abs((int)tpos) % (sizeof(bar_trans) / sizeof(*bar_trans));
		CNM_RECT r;
		Util_SetRect(&r, 0, i, RENDERER_WIDTH, 1);
		Renderer_DrawRect(&r, bar_colors[cidx], bar_trans[tidx], RENDERER_LIGHT);
	}

	_title_card_ticker++;
	for (int i = MAX_TITLE_CARDS - 1; i > -1; i--) {
		int ty = title_target_y + i * 2;
		if (_title_card_ticker > 30 + (MAX_TITLE_CARDS - i - 1) * 3 && title_card_stagger) {
			_title_card_y[i] += (ty - _title_card_y[i]) * 0.6f;
		}
		if (_title_card_ticker > 30 && !title_card_stagger) {
			_title_card_y[i] += (ty - _title_card_y[i]) * 0.25f;
		}
	}
	for (int i = MAX_TITLE_CARDS - 1; i > 0; i--) {
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - (304 / 2), _title_card_y[i], &title_card, 6, RENDERER_LIGHT);
	}
	Renderer_DrawBitmap(RENDERER_WIDTH / 2 - (304 / 2), _title_card_y[0], &title_card, 0, RENDERER_LIGHT);

}

void titlebg_set_card_movement(int target, int delay, int stagger) {
	title_card_stagger = stagger;
	_title_card_ticker = 30 - delay;
	title_target_y = target;
}

