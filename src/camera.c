#include "game.h"
#include "camera.h"
#include "renderer.h"

static int *camx, *camy;
static int cam_in_ybounds, camy_scroll_back, cam_forced;

void Camera_Setup(int x, int y)
{
	cam_in_ybounds = 0;
	camy_scroll_back = 0;
	cam_forced = CNM_FALSE;
	camx = &Game_GetVar(GAME_VAR_CAMERA_X)->data.integer;
	camy = &Game_GetVar(GAME_VAR_CAMERA_Y)->data.integer;
	*camx = x - RENDERER_WIDTH / 2;
	*camy = y - RENDERER_HEIGHT / 2;
}
void Camera_Update(int target_x, int target_y)
{
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
	if (target_y < center_y - 48)
		center_y = target_y + 48;

	if (cam_in_ybounds > 30)
	{
		// Smoothly scroll the center_y value back to the player's y pos
		center_y -= camy_scroll_back++;
		if (center_y < target_y)
			center_y = target_y;
	}

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
void Camera_SetForced(int forced)
{
	cam_forced = forced;
}