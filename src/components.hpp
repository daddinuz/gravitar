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

#pragma once

#include <variant>
#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>
#include <Scene.hpp>

namespace gravitar::components {
    struct Velocity final {
        sf::Vector2f value;
    };

    struct Health final {
        int value;

        [[nodiscard]] inline bool isDead() const noexcept {
            return value <= 0;
        }
    };

    struct Fuel final {
        float value;

        [[nodiscard]] inline bool isOver() const noexcept {
            return value <= 0.0f;
        }
    };

    template<typename T>
    class Supply final {
    public:
        template<typename ...Args>
        explicit Supply(Args &&... args) : mInstance{std::forward<Args>(args)...} {}

        [[nodiscard]] inline T *operator->() noexcept {
            return &mInstance;
        }

        [[nodiscard]] inline const T *operator->() const noexcept {
            return &mInstance;
        }

    private:
        T mInstance;
    };

    class SceneRef final {
    public:
        template<typename ...Args>
        explicit SceneRef(Args &&... args) : mInstance{std::forward<Args>(args)...} {}

        [[nodiscard]] inline SceneId operator*() const noexcept {
            return mInstance;
        }

    private:
        SceneId mInstance;
    };

    template<typename>
    class EntityRef final {
    public:
        template<typename ...Args>
        explicit EntityRef(Args &&... args) : mInstance{std::forward<Args>(args)...} {}

        [[nodiscard]] inline entt::entity operator*() const noexcept {
            return mInstance;
        }

    private:
        entt::entity mInstance;
    };

    class ReloadTime final {
    public:
        explicit ReloadTime(float secondsBeforeShoot);

        void reset();

        void elapse(sf::Time time);

        [[nodiscard]] inline bool canShoot() const noexcept {
            return mElapsed >= mSecondsBeforeShoot;
        }

    private:
        float mElapsed;
        float mSecondsBeforeShoot;
    };

    class HitRadius final {
    public:
        template<typename ...Args>
        explicit HitRadius(Args &&... args) : mInstance{std::forward<Args>(args)...} {}

        [[nodiscard]] inline float operator*() const noexcept {
            return mInstance;
        }

    private:
        float mInstance;
    };

    class Renderable final : public sf::Drawable {
    public:
        template<typename ...Args>
        explicit Renderable(Args &&... args) : mInstance{std::forward<Args>(args)...} {}

        [[nodiscard]] sf::Transformable &operator*() noexcept;
        [[nodiscard]] const sf::Transformable &operator*() const noexcept;

        [[nodiscard]] sf::Transformable *operator->() noexcept;
        [[nodiscard]] const sf::Transformable *operator->() const noexcept;

        template<typename T>
        [[nodiscard]] inline T &as() noexcept {
            return std::get<T>(mInstance);
        }

        template<typename T>
        [[nodiscard]] inline const T &as() const noexcept {
            return std::get<T>(mInstance);
        }

    private:
        void draw(sf::RenderTarget &target, sf::RenderStates states) const final;

        std::variant<sf::Sprite, sf::CircleShape, sf::RectangleShape> mInstance;
    };
}
