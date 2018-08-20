#ifndef __IM_COMMON_H__
#define __IM_COMMON_H__  1


#include "common_types.h"

vec3 blend3(vec3 left, vec3 main, vec3 right, float pos);
vec3 blend2(vec3 left, vec3 right, float pos);
float mix(float a, float b, float k);
float saturatef(float c);
double saturate(double c);
color_t saturatec(color_t c);
float minf(float a, float b, float c);
float maxf(float a, float b, float c);
vec3 vec3_init(float x, float y, float z);
vec3 vec3_mix(vec3 a, vec3 b, float k);

#endif /* __IM_COMMON_H__ */