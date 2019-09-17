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

#include <assets/FontManager.hpp>
#include <scene/Scene.hpp>

namespace gravitar::scene {
    class GameOver final : public Scene {
    public:
        GameOver() = delete; // no default-constructible

        explicit GameOver(const assets::FontManager &fontManager);

        GameOver(const GameOver &) = delete; // no copy-constructible
        GameOver &operator=(const GameOver &) = delete; // no copy-assignable

        GameOver(GameOver &&) = delete; // no move-constructible
        GameOver &operator=(GameOver &&) = delete; // no move-assignable

        void adjustAudio(assets::AudioManager &audioManager) noexcept final;

        SceneId onEvent(const sf::Event &event) noexcept final;

        void update(const sf::RenderWindow &window, sf::Time elapsed) noexcept final;

        void render(sf::RenderTarget &window) const noexcept final;

    private:
        sf::Text mGameOverTitle;
        sf::Text mSpaceLabel;
    };
}
