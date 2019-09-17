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

#include <trace.hpp>
#include <SpriteSheet.hpp>

using namespace gravitar;

SpriteSheet::SpriteSheet(const sf::Texture &texture, Buffer &&buffer) noexcept
        : mBuffer(std::move(buffer)), mTexture(texture) {}

SpriteSheet SpriteSheet::from(const sf::Texture &texture, const sf::Vector2u frameSize, const sf::Vector2u startCoord) {
    const auto[textureWidth, textureHeight] = texture.getSize();
    const auto columns = textureWidth / frameSize.x, rows = textureHeight / frameSize.y;

    if (startCoord.x + columns * frameSize.x > textureWidth || startCoord.y + rows * frameSize.y > textureHeight) {
        throw std::invalid_argument(trace("bad dimensions supplied"));
    }

    auto buffer = Buffer();
    buffer.reserve(rows * columns);

    for (auto row = 0u; row < rows; ++row) {
        const auto top = startCoord.y + row * frameSize.y;

        for (auto column = 0u; column < columns; ++column) {
            buffer.emplace_back(startCoord.x + column * frameSize.x, top, frameSize.x, frameSize.y);
        }
    }

    return SpriteSheet(texture, std::move(buffer));
}

SpriteSheet::const_iterator SpriteSheet::cbegin() const noexcept {
    return mBuffer.cbegin();
}

SpriteSheet::const_iterator SpriteSheet::cend() const noexcept {
    return mBuffer.cend();
}

const SpriteSheet::Buffer &SpriteSheet::getBuffer() const noexcept {
    return mBuffer;
}

const sf::Texture &SpriteSheet::getTexture() const noexcept {
    return mTexture;
}

sf::Sprite SpriteSheet::instanceSprite(const std::size_t frameIndex) const {
    return sf::Sprite(mTexture, mBuffer.at(frameIndex));
}
