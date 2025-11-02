//
// Created by carlo on 01/07/2025.
//

#ifndef SPACEINVADERS3D_GAMETIME_H
#define SPACEINVADERS3D_GAMETIME_H

#include <memory>
#include <chrono>
#include "GameObjectData.h"

class IPlatformServices;
class GameTime {
public:
    GameTime();
    ~GameTime();
    static float deltaTime;
    static void updateTime(IPlatformServices &platformServices);
private:

};
#endif //SPACEINVADERS3D_GAMETIME_H





