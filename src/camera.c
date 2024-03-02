#include <math.h>
#include "game.h"
#include "camera.h"
#include "renderer.h"
#include "wobj.h"
#include "player.h"

static int lastx, lasty;
static int *camx, *camy;
static int cam_in_ybounds, camy_scroll_back, cam_forced, cam_top;
static int cam_ext_target, cam_ext_offset;
static int cam_ext_targety, cam_ext_offsety;

#define DEFAULT_TOP 40

void Camera_Setup(int x, int y)
{
	cam_in_ybounds = 0;
	camy_scroll_back = 0;
	cam_forced = CNM_FALSE;
	cam_top = DEFAULT_TOP;
	camx = &Game_GetVar(GAME_VAR_CAMERA_X)->data.integer;
	camy = &Game_GetVar(GAME_VAR_CAMERA_Y)->data.integer;
	*camx = x - RENDERER_WIDTH / 2;
	*camy = y - RENDERER_HEIGHT / 2;
	cam_ext_target = 0;
	cam_ext_offset = 0;
	cam_ext_targety = 0;
	cam_ext_offsety = 0;
	lastx = *camx;
	lasty = *camy;
}
void Camera_Update(int target_x, int target_y)
{
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	PLAYER_LOCAL_DATA *plr_local = player->local_data;

	lastx = *camx;
	lasty = *camy;
	if (cam_forced)
		return;
	// Update the camera position
	//target_x += RENDERER_WIDTH / 2;
	//target_y += RENDERER_HEIGHT / 2;
	int center_x = *camx + RENDERER_WIDTH / 2;
	int center_y = *camy + RENDERER_HEIGHT / 2;

	if (target_x > center_x + 16)
		center_x = target_x - 16;
	if (target_x < center_x - 16)
		center_x = target_x + 16;
	if (target_y > center_y)
	{
		center_y = target_y;
		cam_in_ybounds = 0;
		camy_scroll_back = 0;
	}
	else
	{
		cam_in_ybounds++;
	}
	int top_target = Wobj_IsGrouneded(player) ? 0 : DEFAULT_TOP;
	if (cam_top < top_target) cam_top = top_target;
	else if (cam_top > top_target) cam_top -= 8;
	if (target_y < center_y - cam_top)
		center_y = target_y + cam_top;

	// Extended camera
	if (fabsf(player->vel_x) > player->speed * 1.5f && !!(player->flags & WOBJ_HFLIP) == player->vel_x < 0.0f) {
		cam_ext_target = player->vel_x < 0.0f ? -64 : 64;
	} else {
		cam_ext_target = 0;
	}
	if (fabsf(player->vel_y) > 14.5f) {
		cam_ext_targety = player->vel_y < 0.0f ? -64 : 64;
	} else {
		cam_ext_targety = 0;
	}
	if (plr_local->vortexed_mode) {
		cam_ext_target = 0;
		cam_ext_targety = 0;
	}
	if (cam_ext_offset < cam_ext_target) cam_ext_offset += 4;
	if (cam_ext_offset > cam_ext_target) cam_ext_offset -= 4;
	if (cam_ext_offsety < cam_ext_targety) cam_ext_offsety += 4;
	if (cam_ext_offsety > cam_ext_targety) cam_ext_offsety -= 4;
	center_x += cam_ext_offset;
	center_y += cam_ext_offsety;

	center_x -= RENDERER_WIDTH / 2;
	center_y -= RENDERER_HEIGHT / 2;

	*camx += (int)(((float)center_x - (float)(*camx)) * 0.5f);
	*camy += (int)(((float)center_y - (float)(*camy)) * 0.5f);
}
void Camera_TeleportPos(int x, int y)
{
	*camx = x - RENDERER_WIDTH / 2;
	*camy = y - RENDERER_HEIGHT / 2;
}
int Camera_GetXPos(void)
{
	return *camx;
}
int Camera_GetYPos(void)
{
	return *camy;
}
int Camera_GetXVel(void)
{
	return *camx - lastx;
}
int Camera_GetYVel(void)
{
	return *camy - lasty;
}
void Camera_SetForced(int forced)
{
	cam_forced = forced;
}
