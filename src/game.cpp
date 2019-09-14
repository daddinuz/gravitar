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

#include "animation.hpp"
#include "components.hpp"
#include "game.hpp"
#include "helpers.hpp"
#include "trace.hpp"

using namespace gravitar;

using Player = entt::tag<"player"_hs>;
using Bunker1 = entt::tag<"bunker1"_hs>;
using Bunker2 = entt::tag<"bunker2"_hs>;
using Bullet = entt::tag<"bullet"_hs>;
using Ground = entt::tag<"ground"_hs>;
using Planet = entt::tag<"planet"_hs>;

constexpr float SPEED = 300.0f;
constexpr float ROTATION_SPEED = 360.0f;

Game &Game::initialize() {
    mFontsManager.initialize();
    mTexturesManager.initialize();
    mSoundTracksManager.initialize();

    mSpaceLabel.initialize(mFontsManager);
    mGravitarTitle.initialize(mTexturesManager);
    mSpriteSheetsManager.initialize(mTexturesManager);

    mWindow.create({800, 600}, "Gravitar", sf::Style::Fullscreen);
    mWindow.setVerticalSyncEnabled(true);
    mWindow.setFramerateLimit(60); // roughly 60 fps
    mWindow.setActive(true);

    mSoundTracksManager.play(SoundTrackId::MainTheme);

    initializeSolarSystemScene();

    return *this;
}

void Game::update() {
    switch (mScene) {
        case Scene::Curtain: updateCurtainScene();
            break;

        case Scene::SolarSystem: updateSolarSystemScene();
            break;

        case Scene::PlanetAssault: updatePlanetAssaultScene();
            break;
    }
}

void Game::run() {
    for (mTimer.restart(); mWindow.isOpen(); mTimer.restart()) {
        mWindow.display();
        mWindow.clear();
        handleGeneralInputs();
        update();
    }
}

void Game::initializeSolarSystemScene() {
    using namespace components;

    mRegistry.group<Renderable, Velocity>();

    auto player = mRegistry.create();
    auto renderable = mSpriteSheetsManager.get(SpriteSheetId::SpaceShip).instanceSprite({0, 0});

    helpers::centerOrigin(renderable, renderable.getLocalBounds());
    renderable.setPosition(400.0f, 300.0f);

    mRegistry.assign<Player>(player);
    mRegistry.assign<Velocity>(player);
    mRegistry.assign<Renderable>(player, std::move(renderable));
}

void Game::handleGeneralInputs() {
    sf::Event event{};

    // flush the events queue (required by SFML in order to work properly)
    while (mWindow.pollEvent(event)) {
        if (sf::Event::Closed == event.type) {
            mWindow.close();
        } else if (sf::Event::KeyPressed == event.type) {
            switch (event.key.code) {
                case sf::Keyboard::Escape: mWindow.close();
                    break;

                case sf::Keyboard::F6: mSoundTracksManager.togglePlaying();
                    break;
// TODO: remove me
#ifndef NDEBUG
                case sf::Keyboard::Delete: mWindow.create({800, 600}, "Gravitar", sf::Style::Close);
                    break;

                case sf::Keyboard::F4: mWindow.create({800, 600}, "Gravitar", sf::Style::Fullscreen);
                    break;
#endif
                default: break;
            }
        }
    }
}

void Game::updateCurtainScene() {
    const auto windowSize = mWindow.getSize();

    mGravitarTitle.setPosition(windowSize.x / 2.0f, windowSize.y / 3.14f);
    mSpaceLabel.setPosition(windowSize.x / 2.0f, windowSize.y / 1.2f);
    mSpaceLabel.update(mTimer.getElapsedTime());

    mWindow.draw(mGravitarTitle);
    mWindow.draw(mSpaceLabel);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        mScene = Scene::SolarSystem;
    }
}

void Game::updateSolarSystemScene() {
    inputSystem();
    motionSystem();
    collisionSystem();
    renderSystem();
}

void Game::updatePlanetAssaultScene() {
    throw std::runtime_error(trace("unimplemented"));
}

void Game::inputSystem() {
    using namespace components;

    mRegistry.view<Player, Renderable, Velocity>().each([this](const auto &player, auto &renderable, auto &velocity) {
        (void) player;

        const auto mousePosition = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow));
        const auto mouseRotation = helpers::rotation(renderable.getPosition(), mousePosition);
        const auto shortestRotation = helpers::shortestRotation(renderable.getRotation(), mouseRotation);

        renderable.rotate(helpers::signum(shortestRotation) * ROTATION_SPEED * mTimer.getElapsedTime().asSeconds());
        *velocity *= 0.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            *velocity += helpers::makeVector2(270.0f, 1.0f);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            *velocity += helpers::makeVector2(180.0f, 1.0f);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            *velocity += helpers::makeVector2(90.0f, 1.0f);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            *velocity += helpers::makeVector2(0.0f, 1.0f);
        }

        *velocity = helpers::normalized({0, 0}, *velocity) * SPEED;
    });
}

void Game::motionSystem() {
    using namespace components;

    mRegistry.group<Renderable, Velocity>().each([this](auto &renderable, const auto &velocity) {
        renderable.move(*velocity * mTimer.getElapsedTime().asSeconds());
    });
}

void Game::collisionSystem() {
    using namespace components;

    const auto viewport = sf::FloatRect(mWindow.getViewport(mWindow.getView()));

    mRegistry.view<Player, Renderable>().each([&viewport](const auto player, auto &renderable) {
        (void) player;

        const auto hitBox = renderable.getHitBox();
        if (not viewport.intersects(hitBox)) {
            renderable.setPosition({viewport.width / 2, viewport.height / 2});
        }
    });
}

void Game::renderSystem() {
    using namespace components;

    mRegistry.view<Renderable>().each([this](const auto &renderable) {
        helpers::debugCall([this, &renderable]() { // display hit-box on debug builds only
            const auto hitBox = renderable.getHitBox();
            auto shape = sf::RectangleShape({hitBox.width, hitBox.height});
            shape.setPosition({hitBox.left, hitBox.top});
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineColor(sf::Color::Red);
            shape.setOutlineThickness(1);
            mWindow.draw(shape);
        });

        mWindow.draw(renderable);
    });
}
