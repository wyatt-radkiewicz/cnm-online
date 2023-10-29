#ifndef _FADEOUT_H_
#define _FADEOUT_H_

void Fadeout_Setup(void);
void Fadeout_FadeToWhite(int fadein, int fade, int fadeout);
void Fadeout_FadeToBlack(int fadein, int fade, int fadeout);
void Fadeout_FadeFromBlack(int init, int fadefrom);
void Fadeout_FadeFromWhite(int init, int fadefrom);
void Fadeout_FadeDeath(int fadein, int fade, int fadeout);
void Fadeout_FadeGameOver(int fadein, int animtime, int fadeout, int fade);
void Fadeout_StepFade(void);
void Fadeout_ApplyFade(void);

#endif
