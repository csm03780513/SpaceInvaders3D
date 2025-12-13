#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "GameObjectData.h"
#include "ecs/components/CombatComponents.h"
#include "json/TinyJson.h"

class IPlatformServices;

namespace ecs {

struct ShipPrefab {
    std::string name;
    Ship ship;
    MainPushConstants render{};
};

struct AlienPrefab {
    std::string name;
    Alien alien;
    MainPushConstants render{};
    std::vector<std::string> modifiers;
};

struct BulletPrefab {
    std::string name;
    BulletType type{BulletType::Ship};
    float speed{1.0f};
    std::array<float, 2> size{Bullet::size, Bullet::size};
    DamagePayload payload{};
};

struct ModifierPrefab {
    std::string name;
    Resistances resistanceDelta{};
    Armor armorDelta{};
    Ailments ailments{};
};

class PrefabLibrary {
public:
    void loadDefaults();
    void loadFromJson(IPlatformServices &platformServices);

    [[nodiscard]] const ShipPrefab &ship(const std::string &name) const;
    [[nodiscard]] const AlienPrefab &alien(const std::string &name) const;
    [[nodiscard]] const BulletPrefab &bullet(const std::string &name) const;
    [[nodiscard]] const ModifierPrefab *modifier(const std::string &name) const;

private:
    void addDefaultShip();
    void addDefaultAlien();
    void addDefaultBullets();

    void parseShips(const TinyJson::Value &root);
    void parseAliens(const TinyJson::Value &root);
    void parseBullets(const TinyJson::Value &root);
    void parseModifiers(const TinyJson::Value &root);

    Resistances parseResistances(const TinyJson::Value &obj) const;
    Armor parseArmor(const TinyJson::Value &obj) const;
    Ailments parseAilments(const TinyJson::Value &obj) const;
    std::optional<DamagePayload> parsePayload(const TinyJson::Value &obj) const;
    std::optional<BulletType> parseBulletType(const std::string &name) const;
    std::optional<AlienMovementType> parseMovement(const std::string &name) const;

    std::unordered_map<std::string, ShipPrefab> ships_{};
    std::unordered_map<std::string, AlienPrefab> aliens_{};
    std::unordered_map<std::string, BulletPrefab> bullets_{};
    std::unordered_map<std::string, ModifierPrefab> modifiers_{};

    ShipPrefab defaultShip_{};
    AlienPrefab defaultAlien_{};
    BulletPrefab defaultBullet_{};
};

} // namespace ecs

