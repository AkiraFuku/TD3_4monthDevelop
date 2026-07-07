#pragma once
#include <cmath>
#include "MathFunction.h"
#include <numbers>

// 経過時間 time [秒] を渡すと 0.0〜1.0 のアルファ値を返す
float BlinkAlpha(float time, float speed = 1.0f);

float BlinkAlpha(float time, float speed, float minA, float maxA);

float GlowAlpha(float time, float speed = 1.0f);

float FadeAlpha(float time, float speed = 1.0f);

