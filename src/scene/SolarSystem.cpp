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

#include <iostream>
#include <tags.hpp>
#include <trace.hpp>
#include <helpers.hpp>
#include <constants.hpp>
#include <components.hpp>
#include <scene/SolarSystem.hpp>

using namespace gravitar;
using namespace gravitar::tags;
using namespace gravitar::scene;
using namespace gravitar::assets;
using namespace gravitar::messages;
using namespace gravitar::constants;
using namespace gravitar::components;

using RandomEngine = helpers::RandomEngine;
using ByteDistribution = helpers::ByteDistribution;
using FloatDistribution = helpers::FloatDistribution;

SolarSystem::SolarSystem(const SceneId youWonSceneId, const SceneId gameOverSceneId) :
        mBuffer{},
        mYouWonSceneId{youWonSceneId},
        mGameOverSceneId{gameOverSceneId} {}

SolarSystem &SolarSystem::initialize(const sf::RenderWindow &window, Assets &assets) noexcept {
    initializePlayers(window, assets);
    initializeReport(assets);
    initializePubSub();
    return *this;
}

void SolarSystem::addPlanet(const sf::RenderWindow &window, helpers::RandomEngine &randomEngine, sf::Color planetColor, SceneId planetSceneId) noexcept {
    const auto[windowWidth, windowHeight] = window.getSize();
    auto planetXDistribution = FloatDistribution(0.0f, windowWidth);
    auto planetYDistribution = FloatDistribution(0.0f, windowHeight);
    auto planetSizeDistribution = FloatDistribution(24, 56);
    const auto planetId = mRegistry.create();
    auto &planetRenderable = mRegistry.assign<Renderable>(planetId, sf::CircleShape(0.0f, 256));
    mRegistry.assign<SceneRef>(planetId, planetSceneId);
    mRegistry.assign<Planet>(planetId);

    auto collides = true;
    for (auto i = 0; collides and i < 128; i++) {
        collides = false;

        auto &circleShape = planetRenderable.as<sf::CircleShape>();
        circleShape.setRadius(planetSizeDistribution(randomEngine));
        helpers::centerOrigin(*planetRenderable, circleShape.getLocalBounds());
        planetRenderable->setPosition(planetXDistribution(randomEngine), planetYDistribution(randomEngine));

        auto &planetHitRadius = mRegistry.assign_or_replace<HitRadius>(planetId, circleShape.getRadius());

        // if planet collides with other entities then retry
        const auto view = mRegistry.view<HitRadius, Renderable>();
        for (const auto entityId : view) {
            if (planetId != entityId) {
                const auto &[entityHitRadius, entityRenderable] = view.get<HitRadius, Renderable>(entityId);

                if (helpers::magnitude(entityRenderable->getPosition(), planetRenderable->getPosition()) <= *planetHitRadius + *entityHitRadius) {
                    collides = true;
                    break;
                }
            }
        }
    }

    if (collides) {
        std::cerr << trace("Unable to generate a random planet") << std::endl;
        std::terminate();
    } else {
        auto &circleShape = planetRenderable.as<sf::CircleShape>();
        circleShape.setFillColor(planetColor);
        circleShape.setOutlineColor(sf::Color(120, 180, 220, 32));
        circleShape.setOutlineThickness(8);
    }
}

SceneId SolarSystem::update(const sf::RenderWindow &window, Assets &assets, const sf::Time elapsed) noexcept {
    (void) assets;
    mNextSceneId = getSceneId();

    if (auto &audioManager = assets.getAudioManager(); SoundTrackId::ComputerF__k != audioManager.getPlaying()) {
        audioManager.play(SoundTrackId::ComputerF__k);
    }

    inputSystem(elapsed);
    motionSystem(elapsed);
    collisionSystem(window);
    livenessSystem();
    reportSystem(window);

    return mNextSceneId;
}

void SolarSystem::render(sf::RenderTarget &window) noexcept {
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

void SolarSystem::operator()(const SolarSystemEntered &message) noexcept {
    const auto planets = mRegistry.view<Planet, SceneRef>();

    for (const auto planetId : planets) {
        if (message.sceneId == *planets.get<SceneRef>(planetId)) {
            const auto players = mRegistry.view<Player>();

            mRegistry.destroy(players.begin(), players.end());
            for (const auto sourcePlayerId : message.registry.view<Player>()) {
                const auto playerId = mRegistry.create(sourcePlayerId, message.registry);
                mRegistry.get<Renderable>(playerId)->setPosition(sf::Vector2f(message.window.getSize()) / 2.0f);
                mRegistry.remove<EntityRef<Tractor>>(playerId);
            }

            if (const auto bunkers = message.registry.view<Bunker>(); bunkers.begin() == bunkers.end()) {
                mRegistry.destroy(planetId);
            }

            return; // we can exit the function because we found our planet and we know that each planet has a unique id.
        }
    }
}

void SolarSystem::initializePubSub() const noexcept {
    pubsub::subscribe<messages::SolarSystemEntered>(*this);
}

void SolarSystem::initializeReport(Assets &assets) noexcept {
    mReport.setCharacterSize(18);
    mReport.setFillColor(sf::Color(105, 235, 245, 255));
    mReport.setFont(assets.getFontsManager().get(FontId::Mechanical));
}

void SolarSystem::initializePlayers(const sf::RenderWindow &window, Assets &assets) noexcept {
    auto playerId = mRegistry.create();
    auto playerRenderable = assets.getSpriteSheetsManager().get(SpriteSheetId::SpaceShip).instanceSprite(0);
    const auto playerBounds = playerRenderable.getLocalBounds();

    helpers::centerOrigin(playerRenderable, playerBounds);
    playerRenderable.setPosition(sf::Vector2f(window.getSize()) / 2.0f);

    mRegistry.assign<Player>(playerId);
    mRegistry.assign<Health>(playerId, PLAYER_HEALTH);
    mRegistry.assign<Fuel>(playerId, PLAYER_FUEL);
    mRegistry.assign<Velocity>(playerId);
    mRegistry.assign<ReloadTime>(playerId, PLAYER_RELOAD_TIME);
    mRegistry.assign<HitRadius>(playerId, std::max(playerBounds.width, playerBounds.height) / 2.0f);
    mRegistry.assign<Renderable>(playerId, std::move(playerRenderable));
}

void SolarSystem::inputSystem(const sf::Time elapsed) noexcept {
    using Key = sf::Keyboard::Key;
    const auto isKeyPressed = &sf::Keyboard::isKeyPressed;

    mRegistry
            .view<Player, Fuel, Velocity, Renderable>()
            .each([&](const auto playerTag, auto &playerFuel, auto &playerVelocity, auto &playerRenderable) {
                (void) playerTag;
                auto speed = PLAYER_SPEED;

                if (isKeyPressed(Key::W)) {
                    speed *= 1.32f;
                } else if (isKeyPressed(Key::S)) {
                    speed *= 0.88f;
                }

                if (isKeyPressed(Key::A)) {
                    playerRenderable->rotate(-PLAYER_ROTATION_SPEED * elapsed.asSeconds());
                }

                if (isKeyPressed(Key::D)) {
                    playerRenderable->rotate(PLAYER_ROTATION_SPEED * elapsed.asSeconds());
                }

                playerVelocity.value = helpers::makeVector2(playerRenderable->getRotation(), speed);
                playerFuel.value -= speed * elapsed.asSeconds();
            });
}

void SolarSystem::motionSystem(const sf::Time elapsed) noexcept {
    mRegistry.view<Velocity, Renderable>().each([&](const auto &velocity, auto &renderable) {
        renderable->move(velocity.value * elapsed.asSeconds());
    });
}

void SolarSystem::collisionSystem(const sf::RenderWindow &window) noexcept {
    const auto viewport = sf::FloatRect(window.getViewport(window.getView()));
    const auto players = mRegistry.view<Player, HitRadius, Renderable>();

    for (const auto playerId : players) {
        const auto &[playerHitRadius, playerRenderable] = players.get<HitRadius, Renderable>(playerId);

        if (viewport.contains(playerRenderable->getPosition())) {
            const auto planets = mRegistry.view<Planet, Renderable, HitRadius, SceneRef>();

            for (const auto planetId : planets) {
                const auto &[planetHitRadius, planetRenderable, planetSceneRef] = planets.get<HitRadius, Renderable, SceneRef>(planetId);

                if (helpers::magnitude(playerRenderable->getPosition(), planetRenderable->getPosition()) <= *playerHitRadius + *planetHitRadius) {
                    mNextSceneId = *planetSceneRef;
                    pubsub::publish<PlanetEntered>(window, mRegistry, *planetSceneRef);
                    return; // we can enter only one planet at a time
                }
            }
        } else {
            mRegistry.get<Health>(playerId).value -= 1;
            playerRenderable->setPosition(sf::Vector2f(window.getSize()) / 2.0f);
        }
    }
}

void SolarSystem::livenessSystem() noexcept {
    auto entitiesToDestroy = std::vector<entt::entity>();

    if (mRegistry.view<Planet>().begin() == mRegistry.view<Planet>().end()) { // no more planets left
        mNextSceneId = mYouWonSceneId;
    }

    mRegistry.view<Player, Health, Fuel>().each([&](const auto id, const auto tag, const auto &health, const auto &fuel) {
        (void) tag;

        if (health.isDead() or fuel.isOver()) {
            entitiesToDestroy.push_back(id);
            mNextSceneId = mGameOverSceneId;
        }
    });

    mRegistry.destroy(entitiesToDestroy.begin(), entitiesToDestroy.end());
}

void SolarSystem::reportSystem(const sf::RenderWindow &window) noexcept {
    mRegistry.view<Player, Health, Fuel>().each([&](const auto tag, const auto &health, const auto &fuel) {
        (void) tag;

        std::snprintf(mBuffer, std::size(mBuffer), "health: %d fuel: %.0f", health.value, fuel.value);
        helpers::centerOrigin(mReport, mReport.getLocalBounds());

        mReport.setString(mBuffer);
        mReport.setPosition(window.getSize().x / 2.0f, 18.0f);
    });
}
