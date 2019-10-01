/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Davide Di Carlo
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <tags.hpp>
#include <pubsub.hpp>
#include <helpers.hpp>
#include <messages.hpp>
#include <components.hpp>
#include <scene/PlanetAssault.hpp>

using namespace gravitar;
using namespace gravitar::tags;
using namespace gravitar::scene;
using namespace gravitar::assets;
using namespace gravitar::messages;
using namespace gravitar::components;

constexpr float SPEED = 180.0f;
constexpr float ROTATION_SPEED = 180.0f;

PlanetAssault::PlanetAssault(const SceneId gameOverSceneId, Assets &assets) :
        mBuffer{},
        mGameOverSceneId{gameOverSceneId} {
    mReport.setCharacterSize(18);
    mReport.setFillColor(sf::Color(105, 235, 245, 255));
    mReport.setFont(assets.getFontsManager().get(FontId::Mechanical));

    mRegistry.group<HitRadius, Renderable>();
    mRegistry.group<Velocity>(entt::get < Renderable > );

    mRegistry.group<Bunker>(entt::get < HitRadius, Renderable > );
    mRegistry.group<Bullet>(entt::get < HitRadius, Renderable > );
    mRegistry.group<Player>(entt::get < HitRadius, Renderable > );
    mRegistry.group<Terrain>(entt::get < HitRadius, Renderable > );
}

SceneId PlanetAssault::update(const sf::RenderWindow &window, Assets &assets, const sf::Time elapsed) noexcept {
    mNextSceneId = getSceneId();

    inputSystem(window, assets.getSpriteSheetsManager(), elapsed);
    motionSystem(elapsed);
    collisionSystem(window);
    livenessSystem();
    reportSystem(window);

    return mNextSceneId;
}

void PlanetAssault::render(sf::RenderTarget &window) noexcept {
    window.draw(mReport);

    mRegistry.view<Renderable>().each([&](const auto id, const auto &renderable) {
        helpers::debug([&]() { // display hit-circle on debug builds only
            if (const auto hitRadius = mRegistry.try_get<HitRadius>(id); hitRadius) {
                auto shape = sf::CircleShape(**hitRadius);
                helpers::centerOrigin(shape, shape.getLocalBounds());
                shape.setPosition(renderable->getPosition());
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(sf::Color::Red);
                shape.setOutlineThickness(1);
                window.draw(shape);
            }
        });

        window.draw(renderable);
    });
}

void PlanetAssault::addBunker(const sf::RenderWindow &window, Assets &assets) noexcept {
    (void) window;

    // TODO randomly generate bunkers
    auto bunkerId = mRegistry.create();
    auto bunkerRenderable = assets.getSpriteSheetsManager().get(SpriteSheetId::Bunker).instanceSprite(0);
    const auto bunkerBounds = bunkerRenderable.getLocalBounds();

    helpers::centerOrigin(bunkerRenderable, bunkerBounds);
    bunkerRenderable.setPosition(400.0f, 300.0f);
    bunkerRenderable.rotate(-90.0f);

    mRegistry.assign<Bunker>(bunkerId);
    mRegistry.assign<Health>(bunkerId, 1);
    mRegistry.assign<HitRadius>(bunkerId, std::max(bunkerBounds.width / 2.0f, bunkerBounds.height / 2.0f));
    mRegistry.assign<Renderable>(bunkerId, std::move(bunkerRenderable));
}

void PlanetAssault::addTerrain(const sf::RenderWindow &window, Assets &assets, std::mt19937 &randomEngine) noexcept {
    (void) window;
    (void) assets;

    using f32_distribution = std::uniform_real_distribution<float>;

    // const auto view = mRegistry.view<Player, Renderable>();
    // const auto playerId = *view.begin();
    // auto &playerRenderable = view.get<Renderable>(playerId);

    // playerRenderable.setPosition({32.0f, 32.0f});
    // const auto playerPosition = playerRenderable.getPosition();
    const auto[halfWindowWidth, halfWindowHeight] = sf::Vector2f(window.getSize()) / 2.0f;

    // const auto quadrant = sf::FloatRect(
    //         (playerPosition.x < halfWindowWidth) ? 0.0f : halfWindowWidth,
    //         (playerPosition.y < halfWindowHeight) ? 0.0f : halfWindowHeight,
    //         halfWindowWidth,
    //         halfWindowHeight
    // );

    // auto startPoint = sf::Vector2f(
    //         0.0f,
    //         f32_distribution(halfWindowHeight, halfWindowHeight * 2.0f)(randomEngine)
    // );

    auto startPoint = sf::Vector2f(128.0f, 128.0f);

    auto endPoint = sf::Vector2f(
            halfWindowWidth * 2.0f,
            f32_distribution(0, halfWindowHeight)(randomEngine)
    );

    (void) endPoint;

    // while (true) {
    auto terrainId = mRegistry.create();
    auto terrainRenderable = assets.getSpriteSheetsManager().get(SpriteSheetId::Terrain).instanceSprite(0);
    const auto terrainBounds = terrainRenderable.getLocalBounds();

    helpers::centerOrigin(terrainRenderable, terrainBounds);
    terrainRenderable.setPosition(startPoint);
    terrainRenderable.rotate(45.0f);

    mRegistry.assign<Terrain>(terrainId);
    mRegistry.assign<HitRadius>(terrainId, std::max(terrainBounds.width / 2.0f, terrainBounds.height / 2.0f));
    mRegistry.assign<Renderable>(terrainId, std::move(terrainRenderable));
    //    break;
    // }
}

void PlanetAssault::setParentSceneId(const SceneId parentSceneId) noexcept {
    mParentSceneId = parentSceneId;
}

SceneId PlanetAssault::getParentSceneId() const noexcept {
    return mParentSceneId;
}

void PlanetAssault::operator()(const PlanetEntered &planetEntered) noexcept {
    if (planetEntered.sceneId == getSceneId()) {
        const auto players = mRegistry.view<Player>();

        mRegistry.destroy(players.begin(), players.end());
        for (const auto playerId : planetEntered.sourceRegistry.view<Player>()) {
            mRegistry.create(playerId, planetEntered.sourceRegistry);
        }
    }
}

void PlanetAssault::inputSystem(const sf::RenderWindow &window, const SpriteSheetsManager &spriteSheetsManager, const sf::Time elapsed) noexcept {
    using Key = sf::Keyboard::Key;
    decltype(auto) keyPressed = &sf::Keyboard::isKeyPressed;

    mRegistry
            .view<Player, Fuel, Velocity, RechargeTime, HitRadius, Renderable>()
            .each([&](const auto playerTag, auto &playerFuel, auto &playerVelocity,
                      auto &playerRechargeTime, const auto &playerHitRadius, auto &playerRenderable) {
                (void) playerTag;

                auto speed = SPEED;
                const auto input = (keyPressed(Key::A) ? 1 : 0) + (keyPressed(Key::D) ? 2 : 0) +
                                   (keyPressed(Key::W) ? 4 : 0) + (keyPressed(Key::S) ? 8 : 0);

                switch (input) {
                    case 1:
                        playerRenderable->rotate(-ROTATION_SPEED * elapsed.asSeconds());
                        break;

                    case 2:
                        playerRenderable->rotate(ROTATION_SPEED * elapsed.asSeconds());
                        break;

                    case 4:
                        speed *= 1.56f;
                        break;

                    case 5:
                        playerRenderable->rotate(-ROTATION_SPEED * 0.92f * elapsed.asSeconds());
                        speed *= 1.32f;
                        break;

                    case 6:
                        playerRenderable->rotate(ROTATION_SPEED * 0.92f * elapsed.asSeconds());
                        speed *= 1.32f;
                        break;

                    case 8:
                        speed *= 0.98f;
                        break;

                    case 9:
                        playerRenderable->rotate(-ROTATION_SPEED * 1.08f * elapsed.asSeconds());
                        speed *= 0.68f;
                        break;

                    case 10:
                        playerRenderable->rotate(ROTATION_SPEED * 1.08f * elapsed.asSeconds());
                        speed *= 0.68f;
                        break;

                    default: {
                        const auto mousePosition = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                        const auto mouseRotation = helpers::rotation(playerRenderable->getPosition(), mousePosition);
                        const auto shortestRotation = helpers::shortestRotation(playerRenderable->getRotation(), mouseRotation);
                        playerRenderable->rotate(static_cast<float>(helpers::signum(shortestRotation)) * ROTATION_SPEED * elapsed.asSeconds());
                    }
                }

                *playerVelocity = helpers::makeVector2(playerRenderable->getRotation(), speed);
                *playerFuel -= speed * elapsed.asSeconds();
                playerRechargeTime.elapse(elapsed);

                if (playerRechargeTime.canShoot() and sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    playerRechargeTime.reset();

                    auto bulletRenderable = spriteSheetsManager.get(SpriteSheetId::Bullet).instanceSprite(0);
                    const auto bulletBounds = bulletRenderable.getLocalBounds();
                    const auto bulletId = mRegistry.create();

                    helpers::centerOrigin(bulletRenderable, bulletBounds);
                    bulletRenderable.setPosition(playerRenderable->getPosition() + helpers::makeVector2(playerRenderable->getRotation(), 2.0f + *playerHitRadius));

                    mRegistry.assign<Bullet>(bulletId);
                    mRegistry.assign<Velocity>(bulletId, helpers::makeVector2(playerRenderable->getRotation(), 800.0f));
                    mRegistry.assign<HitRadius>(bulletId, std::max(bulletBounds.width / 2.0f, bulletBounds.height / 2.0f));
                    mRegistry.assign<Renderable>(bulletId, std::move(bulletRenderable)); // this must be the last line in order to avoid dangling pointers
                }
            });
}

void PlanetAssault::motionSystem(const sf::Time elapsed) noexcept {
    mRegistry.group<Velocity>(entt::get < Renderable > ).each([&](const auto &velocity, auto &renderable) {
        renderable->move(*velocity * elapsed.asSeconds());
    });
}

void PlanetAssault::collisionSystem(const sf::RenderWindow &window) noexcept {
    const auto viewport = sf::FloatRect(window.getViewport(window.getView()));

    const auto players = mRegistry.group<Player>(entt::get < HitRadius, Renderable > );
    for (const auto playerId : players) {
        auto &playerRenderable = players.get<Renderable>(playerId);

        if (not viewport.contains(playerRenderable->getPosition())) {
            mNextSceneId = getParentSceneId();
            playerRenderable->setPosition(sf::Vector2f(window.getSize()) / 2.0f); // TODO handle positioning inside message handling
            pubsub::publish<PlanetExited>(getSceneId(), mRegistry);
            return;
        }
    }

    mRegistry.group<Bullet>(entt::get < HitRadius, Renderable > ).each([&](const auto bulletId, const auto bulletTag, const auto &bulletHitRadius, const auto &bulletRenderable) {
        (void) bulletTag;

        if (viewport.contains(bulletRenderable->getPosition())) {
            auto bulletsToDestroy = std::vector<entt::entity>();

            mRegistry.group<HitRadius, Renderable>().each([&](const auto entityId, const auto &entityHitRadius, const auto &entityRenderable) {
                if (entityId != bulletId and helpers::magnitude(entityRenderable->getPosition(), bulletRenderable->getPosition()) <= *entityHitRadius + *bulletHitRadius) {
                    // TODO handle "raggio traente"
                    bulletsToDestroy.push_back(bulletId);

                    if (mRegistry.has<Player>(entityId) or mRegistry.has<Bunker>(entityId)) {
                        *mRegistry.get<Health>(entityId) -= 1;
                    }
                }
            });

            mRegistry.destroy(bulletsToDestroy.begin(), bulletsToDestroy.end());
        } else {
            mRegistry.destroy(bulletId);
        }
    });

    mRegistry.group<Terrain>(entt::get < HitRadius, Renderable > ).each([&](const auto terrainTag, const auto &terrainHitRadius, const auto &terrainRenderable) {
        (void) terrainTag;

        mRegistry
                .group<Player>(entt::get < HitRadius, Renderable > )
                .each([&](const auto playerId, const auto playerTag, const auto &playerHitRadius, auto &playerRenderable) {
                    (void) playerTag;

                    if (helpers::magnitude(terrainRenderable->getPosition(), playerRenderable->getPosition()) <= *terrainHitRadius + *playerHitRadius) {
                        playerRenderable->setPosition(sf::Vector2f(window.getSize()) / 2.0f);
                        *mRegistry.get<Health>(playerId) -= 1;
                    }
                });
    });

    mRegistry.group<Bunker>(entt::get < HitRadius, Renderable > ).each([&](const auto bunkerTag, const auto &bunkerHitRadius, const auto &bunkerRenderable) {
        (void) bunkerTag;

        mRegistry
                .group<Player>(entt::get < HitRadius, Renderable > )
                .each([&](const auto playerId, const auto playerTag, const auto &playerHitRadius, auto &playerRenderable) {
                    (void) playerTag;

                    if (helpers::magnitude(bunkerRenderable->getPosition(), playerRenderable->getPosition()) <= *bunkerHitRadius + *playerHitRadius) {
                        playerRenderable->setPosition(sf::Vector2f(window.getSize()) / 2.0f);
                        *mRegistry.get<Health>(playerId) -= 1;
                    }
                });
    });
}

void PlanetAssault::livenessSystem() noexcept {
    if (mRegistry.view<Bunker>().begin() == mRegistry.view<Bunker>().end()) { // no more bunkers left
        mNextSceneId = mParentSceneId;
        pubsub::publish<PlanetExited>(getSceneId(), mRegistry);
        pubsub::publish<PlanetDestroyed>(getSceneId());
    }

    mRegistry.view<Player, Health, Fuel>().each([&](const auto id, const auto tag, const auto &health, const auto &fuel) {
        (void) tag;

        if (health.isDead() or fuel.isOver()) {
            mRegistry.destroy(id);
            mNextSceneId = mGameOverSceneId;
        }
    });

    mRegistry.view<Bunker, Health>().each([&](const auto id, const auto tag, const auto &health) {
        (void) tag;

        if (health.isDead()) {
            mRegistry.destroy(id);
        }
    });
}

void PlanetAssault::reportSystem(const sf::RenderWindow &window) noexcept {
    mRegistry.view<Player, Health, Fuel>().each([&](const auto tag, const auto &health, const auto &fuel) {
        (void) tag;

        std::snprintf(mBuffer, std::size(mBuffer), "health: %d    fuel: %3.0f", *health, *fuel);
        helpers::centerOrigin(mReport, mReport.getLocalBounds());

        mReport.setString(mBuffer);
        mReport.setPosition(window.getSize().x / 2.0f, 18.0f);
    });
}
