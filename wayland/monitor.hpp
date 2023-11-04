#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void wayland_motion(double time, double x, double y);
void wayland_button(double time, uint32_t button, uint8_t pressed);
void wayland_key(double time, uint32_t key, uint8_t pressed);
void libinput_motion(double time, double dx, double dy);
void libinput_button(double time, uint32_t button, uint8_t pressed);
void libinput_key(double time, uint32_t key, uint8_t pressed);
void segment_start();
void segment_end();
void hole();
void end();

#ifdef __cplusplus
}
#endif
 
