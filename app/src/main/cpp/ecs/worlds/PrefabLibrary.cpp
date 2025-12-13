#include "PrefabLibrary.h"

#include <algorithm>

#include "json/TinyJson.h"
#include "platform/PlatformServices.h"
#include "Util.h"

namespace ecs {

namespace {
constexpr const char *kDefaultShip = "player";
constexpr const char *kDefaultAlien = "grunt";
constexpr const char *kDefaultBullet = "ship_primary";

std::optional<DamageType> parseDamageType(const std::string &name) {
    if (name == "Kinetic") return DamageType::Kinetic;
    if (name == "Fire") return DamageType::Fire;
    if (name == "Lightning") return DamageType::Lightning;
    if (name == "Cold") return DamageType::Cold;
    if (name == "Poison") return DamageType::Poison;
    if (name == "Radiation") return DamageType::Radiation;
    if (name == "Plasma") return DamageType::Plasma;
    if (name == "DarkMatter") return DamageType::DarkMatter;
    if (name == "Cosmic") return DamageType::Cosmic;
    return std::nullopt;
}

} // namespace

template <typename T>
static T copyOrFallback(const std::unordered_map<std::string, T> &map, const std::string &name, const T &fallback) {
    auto it = map.find(name);
    if (it != map.end()) return it->second;
    return fallback;
}

void PrefabLibrary::loadDefaults() {
    ships_.clear();
    aliens_.clear();
    bullets_.clear();
    modifiers_.clear();

    addDefaultShip();
    addDefaultAlien();
    addDefaultBullets();
}

void PrefabLibrary::loadFromJson(IPlatformServices &platformServices) {
    loadDefaults();

    std::vector<uint8_t> bytes;
    try {
        bytes = platformServices.loadAsset("config/prefabs.json");
    } catch (...) {
        return;
    }
    if (bytes.empty()) return;

    std::string text(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    try {
        const TinyJson::Value root = TinyJson::parse(text);
        if (!root.isObject()) return;
        const auto &obj = root.asObject();

        if (const TinyJson::Value *ships = TinyJson::get(obj, "ships")) parseShips(*ships);
        if (const TinyJson::Value *aliens = TinyJson::get(obj, "aliens")) parseAliens(*aliens);
        if (const TinyJson::Value *bullets = TinyJson::get(obj, "bullets")) parseBullets(*bullets);
        if (const TinyJson::Value *mods = TinyJson::get(obj, "modifiers")) parseModifiers(*mods);
    } catch (...) {
        // keep defaults
    }
}

const ShipPrefab &PrefabLibrary::ship(const std::string &name) const {
    return copyOrFallback(ships_, name, defaultShip_);
}

const AlienPrefab &PrefabLibrary::alien(const std::string &name) const {
    return copyOrFallback(aliens_, name, defaultAlien_);
}

const BulletPrefab &PrefabLibrary::bullet(const std::string &name) const {
    return copyOrFallback(bullets_, name, defaultBullet_);
}

const ModifierPrefab *PrefabLibrary::modifier(const std::string &name) const {
    auto it = modifiers_.find(name);
    if (it == modifiers_.end()) return nullptr;
    return &it->second;
}

void PrefabLibrary::addDefaultShip() {
    ShipPrefab ship{};
    ship.name = kDefaultShip;
    ship.ship.x = 0.0f;
    ship.ship.y = -0.7f;
    ship.ship.widthHeight = Util::getQuadWidthHeight(shipVerts, 6, {1.0f, 1.0f});
    ship.ship.health.hull = 120.0f;
    ship.ship.health.maxHull = 120.0f;
    ship.ship.resistances.byType[(int) DamageType::Kinetic] = 0.10f;
    ship.ship.resistances.byType[(int) DamageType::Fire] = 0.05f;
    ship.ship.resistances.byType[(int) DamageType::Cold] = 0.05f;
    ship.render.texturePos = 0;

    defaultShip_ = ship;
    ships_[ship.name] = ship;
}

void PrefabLibrary::addDefaultAlien() {
    AlienPrefab alien{};
    alien.name = kDefaultAlien;
    alien.alien.x = -0.7f;
    alien.alien.y = 0.8f;
    alien.alien.baseX = -0.7f;
    alien.alien.vy = 0.02f;
    alien.alien.widthHeight = Util::getQuadWidthHeight(alienVerts, 6, {1.0f, 1.0f});
    alien.alien.resistances.byType[(int) DamageType::Kinetic] = 0.10f;
    alien.alien.resistances.byType[(int) DamageType::Fire] = 0.10f;
    alien.alien.resistances.byType[(int) DamageType::Lightning] = 0.05f;
    alien.alien.resistances.byType[(int) DamageType::Cold] = 0.00f;
    alien.alien.resistances.byType[(int) DamageType::Poison] = 0.00f;
    alien.alien.resistances.byType[(int) DamageType::Radiation] = 0.15f;
    alien.alien.resistances.byType[(int) DamageType::Plasma] = 0.05f;
    alien.alien.resistances.byType[(int) DamageType::DarkMatter] = -0.10f;
    alien.alien.resistances.byType[(int) DamageType::Cosmic] = 0.20f;
    alien.render.texturePos = 1;

    defaultAlien_ = alien;
    aliens_[alien.name] = alien;
}

void PrefabLibrary::addDefaultBullets() {
    BulletPrefab shipBullet{};
    shipBullet.name = kDefaultBullet;
    shipBullet.type = BulletType::Ship;
    shipBullet.speed = 2.0f;
    shipBullet.size = {0.04f, 0.05f};
    shipBullet.payload = makePlasma(20.0f);

    BulletPrefab alienBullet{};
    alienBullet.name = "alien_primary";
    alienBullet.type = BulletType::Alien;
    alienBullet.speed = 0.5f;
    alienBullet.size = {0.04f, 0.05f};
    alienBullet.payload = makeKinetic(16.0f);

    defaultBullet_ = shipBullet;
    bullets_[shipBullet.name] = shipBullet;
    bullets_[alienBullet.name] = alienBullet;
}

void PrefabLibrary::parseShips(const TinyJson::Value &root) {
    if (!root.isObject()) return;
    for (const auto &[name, val] : root.asObject()) {
        if (!val.isObject()) continue;
        ShipPrefab prefab{};
        prefab.name = name;
        prefab.ship = defaultShip_.ship;

        const auto &obj = val.asObject();
        if (const TinyJson::Value *health = TinyJson::get(obj, "health")) {
            prefab.ship.health.hull = TinyJson::getNumber(health->asObject(), "hull").value_or(prefab.ship.health.hull);
            prefab.ship.health.maxHull = prefab.ship.health.hull;
            prefab.ship.health.shield = TinyJson::getNumber(health->asObject(), "shield").value_or(prefab.ship.health.shield);
            prefab.ship.health.maxShield = prefab.ship.health.shield;
        }
        if (const TinyJson::Value *res = TinyJson::get(obj, "resistances")) {
            prefab.ship.resistances = parseResistances(*res);
        }
        if (const TinyJson::Value *armor = TinyJson::get(obj, "armor")) {
            prefab.ship.armor = parseArmor(*armor);
        }

        ships_[name] = prefab;
    }
}

void PrefabLibrary::parseAliens(const TinyJson::Value &root) {
    if (!root.isObject()) return;
    for (const auto &[name, val] : root.asObject()) {
        if (!val.isObject()) continue;
        AlienPrefab prefab{};
        prefab.name = name;
        prefab.alien = defaultAlien_.alien;

        const auto &obj = val.asObject();
        if (const TinyJson::Value *movement = TinyJson::get(obj, "movement")) {
            if (auto movementStr = TinyJson::getString(movement->asObject(), "type")) {
                if (auto mt = parseMovement(*movementStr)) {
                    prefab.alien.movementType = *mt;
                }
            }
            prefab.alien.frequency = TinyJson::getNumber(movement->asObject(), "frequency").value_or(prefab.alien.frequency);
            prefab.alien.baseFrequency = prefab.alien.frequency;
            prefab.alien.vy = TinyJson::getNumber(movement->asObject(), "vy").value_or(prefab.alien.vy);
            prefab.alien.amplitude = TinyJson::getNumber(movement->asObject(), "amplitude").value_or(prefab.alien.amplitude);
        }
        if (const TinyJson::Value *res = TinyJson::get(obj, "resistances")) {
            prefab.alien.resistances = parseResistances(*res);
        }
        if (const TinyJson::Value *armor = TinyJson::get(obj, "armor")) {
            prefab.alien.armor = parseArmor(*armor);
        }
        if (const TinyJson::Value *mods = TinyJson::get(obj, "modifiers")) {
            if (mods->isArray()) {
                for (const auto &m : mods->asArray()) {
                    if (m.isString()) prefab.modifiers.push_back(m.asString());
                }
            }
        }

        aliens_[name] = prefab;
    }
}

void PrefabLibrary::parseBullets(const TinyJson::Value &root) {
    if (!root.isObject()) return;
    for (const auto &[name, val] : root.asObject()) {
        if (!val.isObject()) continue;
        BulletPrefab prefab{};
        prefab.name = name;
        prefab.speed = defaultBullet_.speed;
        prefab.payload = defaultBullet_.payload;
        prefab.size = defaultBullet_.size;
        prefab.type = defaultBullet_.type;

        const auto &obj = val.asObject();
        if (auto typeStr = TinyJson::getString(obj, "type")) {
            if (auto bt = parseBulletType(*typeStr)) prefab.type = *bt;
        }
        if (auto speed = TinyJson::getNumber(obj, "speed")) prefab.speed = static_cast<float>(*speed);
        if (auto w = TinyJson::getNumber(obj, "width")) prefab.size[0] = static_cast<float>(*w);
        if (auto h = TinyJson::getNumber(obj, "height")) prefab.size[1] = static_cast<float>(*h);
        if (const TinyJson::Value *payload = TinyJson::get(obj, "payload")) {
            if (auto p = parsePayload(*payload)) prefab.payload = *p;
        }

        bullets_[name] = prefab;
    }
}

void PrefabLibrary::parseModifiers(const TinyJson::Value &root) {
    if (!root.isObject()) return;
    for (const auto &[name, val] : root.asObject()) {
        if (!val.isObject()) continue;
        ModifierPrefab prefab{};
        prefab.name = name;
        const auto &obj = val.asObject();
        if (const TinyJson::Value *res = TinyJson::get(obj, "resistances")) {
            prefab.resistanceDelta = parseResistances(*res);
        }
        if (const TinyJson::Value *armor = TinyJson::get(obj, "armor")) {
            prefab.armorDelta = parseArmor(*armor);
        }
        if (const TinyJson::Value *ail = TinyJson::get(obj, "ailments")) {
            prefab.ailments = parseAilments(*ail);
        }
        modifiers_[name] = prefab;
    }
}

Resistances PrefabLibrary::parseResistances(const TinyJson::Value &obj) const {
    Resistances res{};
    if (!obj.isObject()) return res;
    for (const auto &[k, v] : obj.asObject()) {
        if (!v.isNumber()) continue;
        if (auto dt = parseDamageType(k)) {
            res.byType[(int) *dt] = static_cast<float>(v.asNumber());
        }
    }
    return res;
}

Armor PrefabLibrary::parseArmor(const TinyJson::Value &obj) const {
    Armor armor{};
    if (!obj.isObject()) return armor;
    armor.flatReduction = TinyJson::getNumber(obj.asObject(), "flatReduction").value_or(armor.flatReduction);
    return armor;
}

Ailments PrefabLibrary::parseAilments(const TinyJson::Value &obj) const {
    Ailments ailments{};
    if (!obj.isObject()) return ailments;
    ailments.burnDps = TinyJson::getNumber(obj.asObject(), "burnDps").value_or(ailments.burnDps);
    ailments.poisonDps = TinyJson::getNumber(obj.asObject(), "poisonDps").value_or(ailments.poisonDps);
    ailments.radiationDps = TinyJson::getNumber(obj.asObject(), "radiationDps").value_or(ailments.radiationDps);
    ailments.slowFactor = TinyJson::getNumber(obj.asObject(), "slowFactor").value_or(ailments.slowFactor);
    ailments.slowTtl = TinyJson::getNumber(obj.asObject(), "slowTtl").value_or(ailments.slowTtl);
    ailments.stunned = TinyJson::getNumber(obj.asObject(), "stunned").value_or(ailments.stunned ? 1.0 : 0.0) > 0.0f;
    ailments.stunTtl = TinyJson::getNumber(obj.asObject(), "stunTtl").value_or(ailments.stunTtl);
    ailments.shredAmount = TinyJson::getNumber(obj.asObject(), "shredAmount").value_or(ailments.shredAmount);
    ailments.shredTtl = TinyJson::getNumber(obj.asObject(), "shredTtl").value_or(ailments.shredTtl);
    return ailments;
}

std::optional<DamagePayload> PrefabLibrary::parsePayload(const TinyJson::Value &obj) const {
    if (!obj.isArray()) return std::nullopt;
    DamagePayload payload{};
    for (const auto &slice : obj.asArray()) {
        if (!slice.isObject()) continue;
        const auto &sObj = slice.asObject();
        const auto typeStr = TinyJson::getString(sObj, "type");
        const auto amount = TinyJson::getNumber(sObj, "amount");
        if (!typeStr.has_value() || !amount.has_value()) continue;
        if (auto dt = parseDamageType(*typeStr)) {
            payload.slices.push_back({*dt, static_cast<float>(*amount)});
        }
    }
    payload.critChance = 0.0f;
    payload.critMult = 1.0f;
    return payload;
}

std::optional<BulletType> PrefabLibrary::parseBulletType(const std::string &name) const {
    if (name == "Ship") return BulletType::Ship;
    if (name == "Alien") return BulletType::Alien;
    return std::nullopt;
}

std::optional<AlienMovementType> PrefabLibrary::parseMovement(const std::string &name) const {
    if (name == "SnakeWave") return AlienMovementType::SnakeWave;
    if (name == "JustGoDown") return AlienMovementType::JustGoDown;
    if (name == "TogetherOne") return AlienMovementType::TogetherOne;
    if (name == "SineWave") return AlienMovementType::SineWave;
    if (name == "Circle") return AlienMovementType::Circle;
    if (name == "LeftRight") return AlienMovementType::LeftRight;
    if (name == "MySnakeWave") return AlienMovementType::MySnakeWave;
    return std::nullopt;
}

} // namespace ecs

