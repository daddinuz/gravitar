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

#include <helpers.hpp>
#include <components.hpp>
#include <scene/SolarSystem.hpp>

using namespace gravitar::scene;
using namespace gravitar::assets;
using namespace gravitar::components;

using Bullet = entt::tag<"bullet"_hs>;
using Player = entt::tag<"player"_hs>;

constexpr float SPEED = 180.0f;
constexpr float ROTATION_SPEED = 180.0f;

SolarSystem::SolarSystem(const SpriteSheetManager &spriteSheetManager) : mSpriteSheetManager(spriteSheetManager) {
    mRegistry.group<Renderable, Velocity>();

    auto player = mRegistry.create();
    auto renderable = mSpriteSheetManager.get(SpriteSheetId::SpaceShip).instanceSprite(0);

    helpers::centerOrigin(renderable, renderable.getLocalBounds());
    renderable.setPosition(400.0f, 300.0f);

    mRegistry.assign<Player>(player);
    mRegistry.assign<Health>(player, 3);
    mRegistry.assign<Fuel>(player, 20000);
    mRegistry.assign<Velocity>(player);
    mRegistry.assign<RechargeTime>(player, RechargeTime(1.0f));
    mRegistry.assign<Renderable>(player, std::move(renderable));
}

void SolarSystem::update(const sf::RenderWindow &window, sf::Time elapsed) {
    inputSystem(window, elapsed);
    motionSystem(elapsed);
    collisionSystem(window);
}

void SolarSystem::render(sf::RenderTarget &renderTarget) {
    mRegistry.view<Renderable>().each([&](const auto &renderable) {
        helpers::debug([&]() { // display hit-box on debug builds only
            const auto hitBox = renderable.getHitBox();
            auto shape = sf::RectangleShape({hitBox.width, hitBox.height});
            shape.setPosition({hitBox.left, hitBox.top});
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineColor(sf::Color::Red);
            shape.setOutlineThickness(1);
            renderTarget.draw(shape);
        });

        renderTarget.draw(renderable);
    });
}

void SolarSystem::inputSystem(const sf::RenderWindow &window, sf::Time elapsed) {
    mRegistry.view<Player, Renderable, Velocity, Fuel, RechargeTime>().each([&](const auto &player, auto &renderable, auto &velocity, auto &fuel, auto &rechargeTime) {
        (void) player;
        auto speed = SPEED;
        const auto input = (sf::Keyboard::isKeyPressed(sf::Keyboard::A) ? 1 : 0) + (sf::Keyboard::isKeyPressed(sf::Keyboard::D) ? 2 : 0) +
                           (sf::Keyboard::isKeyPressed(sf::Keyboard::W) ? 4 : 0) + (sf::Keyboard::isKeyPressed(sf::Keyboard::S) ? 8 : 0);

        switch (input) {
            case 0: {
                const auto mousePosition = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                const auto mouseRotation = helpers::rotation(renderable.getPosition(), mousePosition);
                const auto shortestRotation = helpers::shortestRotation(renderable.getRotation(), mouseRotation);
                renderable.rotate(static_cast<float>(helpers::signum(shortestRotation)) * ROTATION_SPEED * elapsed.asSeconds());
            }
                break;

            case 1:
                renderable.rotate(-ROTATION_SPEED * elapsed.asSeconds());
                break;

            case 2:
                renderable.rotate(ROTATION_SPEED * elapsed.asSeconds());
                break;

            case 4:
                speed *= 1.56f;
                break;

            case 5:
                renderable.rotate(-ROTATION_SPEED * 0.92f * elapsed.asSeconds());
                speed *= 1.32f;
                break;

            case 6:
                renderable.rotate(ROTATION_SPEED * 0.92f * elapsed.asSeconds());
                speed *= 1.32f;
                break;

            case 8:
                speed *= 0.98f;
                break;

            case 9:
                renderable.rotate(-ROTATION_SPEED * 1.08f * elapsed.asSeconds());
                speed *= 0.68f;
                break;

            case 10:
                renderable.rotate(ROTATION_SPEED * 1.08f * elapsed.asSeconds());
                speed *= 0.68f;
                break;

            default:
                break;
        }

        *velocity = helpers::makeVector2(renderable.getRotation(), speed);
        *fuel -= speed * elapsed.asSeconds();
        rechargeTime.elapse(elapsed);

        if (rechargeTime.canShoot() and sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            rechargeTime.reset();
            const auto bulletEntity = mRegistry.create();
            auto bulletRenderable = mSpriteSheetManager.get(SpriteSheetId::Bullet).instanceSprite(0);

            helpers::centerOrigin(bulletRenderable, bulletRenderable.getLocalBounds());
            bulletRenderable.setPosition(renderable.getPosition() + helpers::makeVector2(renderable.getRotation(), 22.8f));

            mRegistry.assign<Bullet>(bulletEntity);
            mRegistry.assign<Velocity>(bulletEntity, helpers::makeVector2(renderable.getRotation(), 800.0f));
            mRegistry.assign<Renderable>(bulletEntity, std::move(bulletRenderable)); // this must be the last line in order to avoid dangling pointers
        }
    });
}

void SolarSystem::motionSystem(sf::Time elapsed) {
    mRegistry.group<Renderable, Velocity>().each([&](auto &renderable, const auto &velocity) {
        renderable.move(*velocity * elapsed.asSeconds());
    });
}

void SolarSystem::collisionSystem(const sf::RenderWindow &window) {
    const auto viewport = sf::FloatRect(window.getViewport(window.getView()));

    mRegistry.view<Player, Renderable>().each([&](const auto &entity, const auto &player, auto &renderable) {
        (void) player;

        if (not viewport.intersects(renderable.getHitBox())) {
            *mRegistry.get<Health>(entity) -= 1;
            renderable.setPosition({viewport.width / 2, viewport.height / 2});
        }
    });

    mRegistry.view<Bullet, Renderable>().each([&](const auto &bulletEntity, const auto &bullet, const auto &bulletRenderable) {
        (void) bullet;

        if (not viewport.intersects(bulletRenderable.getHitBox())) {
            mRegistry.destroy(bulletEntity);
        } else {
            mRegistry.view<Player, Renderable>().each([&](const auto &playerEntity, const auto &player, auto &playerRenderable) {
                (void) player;

                if (bulletRenderable.getHitBox().intersects(playerRenderable.getHitBox())) {
                    mRegistry.destroy(bulletEntity);
                    *mRegistry.get<Health>(playerEntity) -= 1;
                }
            });
        }
    });
}
