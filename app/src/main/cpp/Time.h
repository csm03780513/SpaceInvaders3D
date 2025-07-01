//
// Created by carlo on 01/07/2025.
//

#ifndef SPACEINVADERS3D_TIME_H
#define SPACEINVADERS3D_TIME_H

#include "android_native_app_glue.h"
#include <memory>
#include <chrono>
#include "GameObjectData.h"
class Time {
public:
    Time();
    ~Time();
    static float deltaTime;
    static void updateTime();
private:

};
#endif //SPACEINVADERS3D_TIME_H





