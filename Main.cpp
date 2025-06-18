#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <complex>
#include <vector>
#include <cmath>
#include <iostream>
#include <functional>

// Generate a sine wave buffer for the sound
sf::SoundBuffer generateSineBuffer(int sampleRate, float duration, float frequency) {
    int count = static_cast<int>(sampleRate * duration);
    std::vector<sf::Int16> samples(count);
    for (int i = 0; i < count; ++i) {
        samples[i] = static_cast<sf::Int16>(32760 * std::sin(2 * M_PI * frequency * i / sampleRate));
    }
    sf::SoundBuffer buffer;
    buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate);
    return buffer;
}

// Helper to map screen to complex plane
std::complex<float> screenToComplex(int x, int y, float zoom, sf::Vector2f offset, int width, int height) {
    return std::complex<float>(
        (x + offset.x - width / 2.f) / zoom,
        (y + offset.y - height / 2.f) / zoom
    );
}

// Formula definitions
std::complex<float> formula1(const std::complex<float>& z, const std::complex<float>& c) {
    // abs(re(z^2)) + i * im(z^2) + c
    float re2 = z.real() * z.real() - z.imag() * z.imag();
    float im2 = 2 * z.real() * z.imag();
    return std::complex<float>(std::abs(re2), im2) + c;
}
std::complex<float> formula2(const std::complex<float>& z, const std::complex<float>& c) {
    // abs(re(z^2)) + i * abs(im(z^2)) + c
    float re2 = z.real() * z.real() - z.imag() * z.imag();
    float im2 = 2 * z.real() * z.imag();
    return std::complex<float>(std::abs(re2), std::abs(im2)) + c;
}
std::complex<float> formula3(const std::complex<float>& z, const std::complex<float>& c) {
    // re(z^2) - i * im(z^2) + c
    float re2 = z.real() * z.real() - z.imag() * z.imag();
    float im2 = 2 * z.real() * z.imag();
    return std::complex<float>(re2, -im2) + c;
}
std::complex<float> formula4(const std::complex<float>& z, const std::complex<float>& c) {
    // abs(Re(z) * abs(Re(z)) + Im(z)^2) + 2i * Re(z) * Im(z) + c
    float re_part = z.real() * std::abs(z.real()) + z.imag() * z.imag();
    float im_part = 2 * z.real() * z.imag();
    return std::complex<float>(std::abs(re_part), im_part) + c;
}

int main() {
    const int width = 800;
    const int height = 600;
    const int maxIter = 100;
    float zoom = 250.0f;
    sf::Vector2f offset(0.f, 0.f);

    sf::RenderWindow window(sf::VideoMode(width, height), "Celtic Orbit Explorer (Zoom, Pan, Mouse-Direct Orbit Period, Julia/J-explore, Formula Switch 1-4)");
    sf::Image fractalImage;
    fractalImage.create(width, height, sf::Color::Black);

    // Julia mode state
    bool juliaMode = false;
    std::complex<float> juliaC(0, 0);

    // Formulas vector and current index
    std::vector<std::function<std::complex<float>(const std::complex<float>&, const std::complex<float>&)>> formulas = {
        formula1, formula2, formula3, formula4
    };
    int formulaIndex = 0;

    // Precompute fractal image based on zoom and offset
    auto computeFractal = [&](float zoom, sf::Vector2f offset, bool juliaMode, std::complex<float> juliaC, int formulaIndex) {
        for (int px = 0; px < width; ++px) {
            for (int py = 0; py < height; ++py) {
                std::complex<float> c = screenToComplex(px, py, zoom, offset, width, height);
                std::complex<float> z = juliaMode ? c : c;
                std::complex<float> cc = juliaMode ? juliaC : c;
                int iter = 0;
                for (; iter < maxIter; ++iter) {
                    z = formulas[formulaIndex](z, cc);
                    if (std::abs(z) > 2.0f) break;
                }
                sf::Uint8 color = static_cast<sf::Uint8>(255 * iter / maxIter);
                fractalImage.setPixel(px, py, sf::Color(color, color, color));
            }
        }
    };

    computeFractal(zoom, offset, juliaMode, juliaC, formulaIndex);
    sf::Texture fractalTexture;
    fractalTexture.loadFromImage(fractalImage);
    sf::Sprite fractalSprite(fractalTexture);

    sf::Sound sound;
    sf::SoundBuffer buffer;

    int lastPeriod = -1; // To avoid printing the same period too many times

    bool needsUpdate = false;
    const float zoomFactor = 1.2f; // Controls zoom speed

    // Camera drag state
    bool dragging = false;
    sf::Vector2i lastMousePos;
    sf::Vector2f dragStartOffset;

    // For period display
    int mousePeriod = -1;
    std::vector<std::complex<float>> mouseOrbit;

    // Formula info display
    std::vector<std::string> formulaNames = {
        "abs(re(z^2)) + i * im(z^2) + c",
        "abs(re(z^2)) + i * abs(im(z^2)) + c",
        "re(z^2) - i * im(z^2) + c",
        "abs(Re(z) * abs(Re(z)) + Im(z)^2) + 2i * Re(z) * Im(z) + c"
    };

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Mouse wheel zooming
            if (event.type == sf::Event::MouseWheelScrolled) {
                sf::Vector2i mouse = sf::Mouse::getPosition(window);
                std::complex<float> beforeZoom = screenToComplex(mouse.x, mouse.y, zoom, offset, width, height);

                if (event.mouseWheelScroll.delta > 0) {
                    zoom *= zoomFactor;
                } else if (event.mouseWheelScroll.delta < 0) {
                    zoom /= zoomFactor;
                }

                // Keep the point under the mouse stationary
                std::complex<float> afterZoom = screenToComplex(mouse.x, mouse.y, zoom, offset, width, height);
                offset.x += (afterZoom.real() - beforeZoom.real()) * zoom;
                offset.y += (afterZoom.imag() - beforeZoom.imag()) * zoom;

                needsUpdate = true;
            }

            // ALT + LMB drag start
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left &&
                (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))) {
                dragging = true;
                lastMousePos = sf::Mouse::getPosition(window);
                dragStartOffset = offset;
            }

            // ALT + LMB drag end
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                dragging = false;
            }

            // If window loses focus, stop dragging
            if (event.type == sf::Event::LostFocus) {
                dragging = false;
            }

            // Formula switching with 1-4
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) {
                    formulaIndex = 0; needsUpdate = true;
                    std::cout << "Switched to formula 1: " << formulaNames[0] << std::endl;
                }
                if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
                    formulaIndex = 1; needsUpdate = true;
                    std::cout << "Switched to formula 2: " << formulaNames[1] << std::endl;
                }
                if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
                    formulaIndex = 2; needsUpdate = true;
                    std::cout << "Switched to formula 3: " << formulaNames[2] << std::endl;
                }
                if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::Numpad4) {
                    formulaIndex = 3; needsUpdate = true;
                    std::cout << "Switched to formula 4: " << formulaNames[3] << std::endl;
                }
            }
        }

        // Camera dragging logic
        if (dragging && (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))) {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            sf::Vector2i delta = mouse - lastMousePos;
            offset = dragStartOffset - sf::Vector2f(delta.x, delta.y);
            needsUpdate = true;
        }

        // --- Julia mode handling ---
        bool newJuliaMode = sf::Keyboard::isKeyPressed(sf::Keyboard::J);
        if (newJuliaMode && !juliaMode) {
            // Just entered Julia mode, set Julia point to mouse
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            juliaC = screenToComplex(mouse.x, mouse.y, zoom, offset, width, height);
            needsUpdate = true;
        } else if (newJuliaMode && juliaMode) {
            // While holding J, update Julia point to mouse
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            juliaC = screenToComplex(mouse.x, mouse.y, zoom, offset, width, height);
            needsUpdate = true;
        }
        juliaMode = newJuliaMode;

        // --- Get orbit period at mouse at all times ---
        sf::Vector2i mouse = sf::Mouse::getPosition(window);
        mousePeriod = -1;
        mouseOrbit.clear();
        if (mouse.x >= 0 && mouse.x < width && mouse.y >= 0 && mouse.y < height) {
            std::complex<float> c = screenToComplex(mouse.x, mouse.y, zoom, offset, width, height);
            std::complex<float> z, cc;
            if (juliaMode) {
                z = c;
                cc = juliaC;
            } else {
                z = c;
                cc = c;
            }
            int period = 0;
            int maxOrbit = 1000;
            std::vector<std::complex<float>> orbit;
            bool found = false;
            for (; period < maxOrbit; ++period) {
                z = formulas[formulaIndex](z, cc);
                orbit.push_back(z);
                // check for repetition (simple period detection)
                for (int j = 0; j < period; ++j) {
                    if (std::abs(z - orbit[j]) < 1e-4) {
                        found = true;
                        break;
                    }
                }
                if (found || std::abs(z) > 2.0f) break;
            }
            mousePeriod = period;
            mouseOrbit = orbit;
        }

        if (needsUpdate) {
            computeFractal(zoom, offset, juliaMode, juliaC, formulaIndex);
            fractalTexture.loadFromImage(fractalImage);
            fractalSprite.setTexture(fractalTexture, true);
            needsUpdate = false;
        }

        window.clear();
        window.draw(fractalSprite);

        // Draw Julia point marker if in Julia mode
        if (juliaMode) {
            float x = juliaC.real() * zoom + width / 2.f - offset.x;
            float y = juliaC.imag() * zoom + height / 2.f - offset.y;
            sf::CircleShape juliaMarker(8.f);
            juliaMarker.setFillColor(sf::Color::Blue);
            juliaMarker.setOrigin(8.f, 8.f);
            juliaMarker.setPosition(x, y);
            window.draw(juliaMarker);
        }

        // Show orbit and period on the mouse at all times
        if (mouse.x >= 0 && mouse.y >= 0 && mouse.x < width && mouse.y < height) {
            // Print the period to console only if it changed
            if (mousePeriod != lastPeriod) {
                if (juliaMode) {
                    std::cout << "Julia orbit period (" << juliaC.real() << "," << juliaC.imag() << ") [" << (formulaIndex+1) << "]: " << mousePeriod << std::endl;
                } else {
                    std::cout << "Orbit period [" << (formulaIndex+1) << "]: " << mousePeriod << std::endl;
                }
                lastPeriod = mousePeriod;
            }

            // Draw a circle at the mouse position
            sf::CircleShape marker(8.f);
            marker.setFillColor(sf::Color::Red);
            marker.setOrigin(8.f, 8.f);
            marker.setPosition(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            window.draw(marker);

            // Draw the orbit path
            if (mouseOrbit.size() > 1) {
                sf::VertexArray orbitLine(sf::LineStrip, mouseOrbit.size());
                for (size_t i = 0; i < mouseOrbit.size(); ++i) {
                    float x = mouseOrbit[i].real() * zoom + width / 2.f - offset.x;
                    float y = mouseOrbit[i].imag() * zoom + height / 2.f - offset.y;
                    orbitLine[i].position = sf::Vector2f(x, y);
                    orbitLine[i].color = sf::Color::Green;
                }
                window.draw(orbitLine);
            }

            // Play a tone where period affects pitch (frequency) if left mouse is held (without ALT)
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
                !(sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))) {
                float freq = 220.0f + (mousePeriod % 40) * 10.0f; // Vary pitch by period
                buffer = generateSineBuffer(44100, 0.08f, freq);
                sound.setBuffer(buffer);
                sound.play();
            }
        } else {
            lastPeriod = -1;
        }

        window.display();
    }
    return 0;
}