#ifndef _CAMERA_H_
#define _CAMERA_H_

void Camera_Setup(int x, int y);
void Camera_Update(int target_x, int target_y);
void Camera_TeleportPos(int x, int y);
int Camera_GetXPos(void);
int Camera_GetYPos(void);
int Camera_GetXVel(void);
int Camera_GetYVel(void);
void Camera_SetForced(int forced);

#endif
