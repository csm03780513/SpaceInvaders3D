//
// Created by carlo on 26/10/2025.
//

#ifndef SPACEINVADERS3D_DAMAGEVIZ_H
#define SPACEINVADERS3D_DAMAGEVIZ_H

// DamageViz.hpp
#pragma once
#include <cstdint>
#include "Damage.h"

//inline uint32_t colorFor(DamageType t) {
//    switch (t) {
//        case DamageType::Kinetic:    return 0xFFB0B0B0; // grey
//        case DamageType::Fire:       return 0xFFFF5A2A; // orange/red
//        case DamageType::Lightning:  return 0xFF4EB7FF; // blue
//        case DamageType::Cold:       return 0xFF6FD0FF; // icy blue
//        case DamageType::Poison:     return 0xFF69D17D; // green
//        case DamageType::Radiation:  return 0xFFE8D83C; // yellow
//        case DamageType::Plasma:     return 0xFFFFFFFF; // white
//        case DamageType::DarkMatter: return 0xFFB178FF; // purple
//        case DamageType::Cosmic:     return 0xFF00FFE5; // teal
//    }
//    return 0xFFFFFFFF;
//}

inline glm::vec4 colorFor(DamageType t) {
    switch (t) {
        case DamageType::Kinetic:    return {0.69f, 0.69f, 0.69f, 1.0f}; // grey
        case DamageType::Fire:       return {1.00f, 0.50f, 0.20f,1.0f}; // orange/red
        case DamageType::Lightning:  return {0.31f, 0.72f, 1.00f, 1.0f}; // electric blue
        case DamageType::Cold:       return {0.44f, 0.82f, 1.00f, 1.0f}; // icy blue
        case DamageType::Poison:     return {0.41f, 0.82f, 0.49f, 1.0f}; // toxic green
        case DamageType::Radiation:  return {0.91f, 0.85f, 0.23f, 1.0f}; // yellow
        case DamageType::Plasma:     return {1.00f, 1.00f, 1.00f, 1.0f}; // white
        case DamageType::DarkMatter: return {0.69f, 0.47f, 1.00f, 1.0f}; // purple
        case DamageType::Cosmic:     return {0.00f, 1.00f, 0.90f, 1.0f}; // teal
        default:                     return {1.00f, 1.00f, 1.00f, 1.0f};
    }
}


class DamageViz {

};


#endif //SPACEINVADERS3D_DAMAGEVIZ_H
