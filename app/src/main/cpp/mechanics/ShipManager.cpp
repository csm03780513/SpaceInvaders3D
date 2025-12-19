#include "ShipManager.h"

#include "../Util.h"
#include "ProjectileManager.h"
#include "../PowerUpManager.h"
#include "../SFXMixer.h"

namespace {
constexpr float kShipYOffset = 0.12f;
}

Ship &ShipManager::ship() {
    return ship_;
}

const Ship &ShipManager::ship() const {
    return ship_;
}

MainPushConstants &ShipManager::pushConstants() {
    return shipPC_;
}

const MainPushConstants &ShipManager::pushConstants() const {
    return shipPC_;
}

void ShipManager::initialize(const std::shared_ptr<Util> &util, const std::shared_ptr<SFXMixer> &sfxMixer) {
    Resistances enemyRes{};
    enemyRes.byType[(int)DamageType::Kinetic] = 0.10f;
    enemyRes.byType[(int)DamageType::Fire] = 0.10f;
    enemyRes.byType[(int)DamageType::Lightning] = 0.05f;
    enemyRes.byType[(int)DamageType::Cold] = 0.00f;
    enemyRes.byType[(int)DamageType::Poison] = 0.00f;
    enemyRes.byType[(int)DamageType::Radiation] = 0.15f;
    enemyRes.byType[(int)DamageType::Plasma] = 0.05f;
    enemyRes.byType[(int)DamageType::DarkMatter] = -0.10f;
    enemyRes.byType[(int)DamageType::Cosmic] = 0.20f;

    ship_.resistances = enemyRes;
    ship_.widthHeight = Util::getQuadWidthHeight(quadVerts, 6, {1, 1});
    shipPC_.pos = {ship_.x, ship_.y};
    shipPC_.rotation = 0.0f;
    sfxMixer_ = sfxMixer.get();
    (void)util;
}

void ShipManager::setInput(float x, float y, bool fireBullet,
                           ProjectileManager *projectiles, PowerUpManager *powerUps) {
    ship_.x = x;
    ship_.y = y - kShipYOffset;
    ship_.color[0] = ship_.x;
    shipPC_.pos = {ship_.x, ship_.y};

    if (!fireBullet || !projectiles || !powerUps) {
        return;
    }

    if (canFire_ && sfxMixer_) {
//        sfxMixer_->playClip("shoot",0.0f);
    }
    projectiles->spawnShipBullets({ship_.x, ship_.y}, powerUps->doubleShotActive, canFire_);
}

void ShipManager::update(float dt) {
    if (lastFireTime_ > rateOfFire_) {
        lastFireTime_ = 0.0f;
        canFire_ = true;
    } else {
        lastFireTime_ += dt;
        canFire_ = false;
    }

    shipPC_.flashAmount -= dt * 5.0f;
    if (shipPC_.flashAmount < 0.0f) {
        shipPC_.flashAmount = 0.0f;
    }
}

void ShipManager::resetForNewGame(bool alienBelow) {
    if (ship_.health.dead || alienBelow) {
        ship_.health.hull = 100.0f;
        ship_.health.dead = false;
    }
    lastFireTime_ = 0.0f;
    canFire_ = false;
    shipPC_.flashAmount = 0.0f;
    shipPC_.pos = {ship_.x, ship_.y};
}

bool ShipManager::isDead() const {
    return ship_.health.dead;
}
