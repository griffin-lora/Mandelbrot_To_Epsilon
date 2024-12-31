#pragma once
#include <cglm/types-struct.h>

void init_camera(void);
void update_camera(void);
mat3s get_affine_map(float delta);