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

#include <components.hpp>

using namespace gravitar::components;

/*
 * SceneSwitcher
 */

SceneSwitcher::SceneSwitcher(gravitar::SceneId sceneId) : mSceneId(sceneId) {}

/*
 * RechargeTime
 */

RechargeTime::RechargeTime(float secondsBeforeShoot) : mElapsed(secondsBeforeShoot), mSecondsBeforeShoot(secondsBeforeShoot) {}

void RechargeTime::reset() {
    mElapsed = 0;
}

void RechargeTime::elapse(const sf::Time &time) {
    if (not canShoot()) {
        mElapsed += time.asSeconds();
    }
}

/*
 * HitRadius
 */

HitRadius::HitRadius(const float value) : mRadius(value) {}

/*
 * Renderable
 */

Renderable::Renderable(sf::Sprite &&instance) : mInstance(std::move(instance)) {}

Renderable::Renderable(sf::CircleShape &&instance) : mInstance(std::move(instance)) {}

void Renderable::rotate(float angle) {
    std::visit([&angle](auto &instance) { instance.rotate(angle); }, mInstance);
}

float Renderable::getRotation() const noexcept {
    return std::visit([](const auto &instance) { return instance.getRotation(); }, mInstance);
}

void Renderable::move(const sf::Vector2f &offset) {
    std::visit([&offset](auto &instance) { instance.move(offset); }, mInstance);
}

void Renderable::setPosition(const sf::Vector2f &position) {
    std::visit([&position](auto &instance) { instance.setPosition(position); }, mInstance);
}

sf::Vector2f Renderable::getPosition() const noexcept {
    return std::visit([](const auto &instance) { return instance.getPosition(); }, mInstance);
}

sf::Vector2f Renderable::getOrigin() const noexcept {
    return std::visit([](const auto &instance) { return instance.getOrigin(); }, mInstance);
}

void Renderable::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    (void) states;
    std::visit([&target](const auto &instance) { target.draw(instance); }, mInstance);
}

Renderable::Instance &Renderable::operator*() noexcept {
    return mInstance;
}

const Renderable::Instance &Renderable::operator*() const noexcept {
    return mInstance;
}
