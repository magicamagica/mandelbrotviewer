#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <thread>
#include <atomic>

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_ITER = 1200; // Increase / Decrease iterations for less/more visualization

sf::Color mandelbrotColor(int n) {
    if (n == MAX_ITER) return sf::Color::Black;
    float t = static_cast<float>(n) / MAX_ITER;
    return sf::Color(
        static_cast<sf::Uint8>(9 * (1 - t) * t * t * t * 255),
        static_cast<sf::Uint8>(15 * (1 - t) * (1 - t) * t * t * 255),
        static_cast<sf::Uint8>(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255)
    );
}

void computeMandelbrot(sf::Uint8* pixels, int startY, int endY, double zoom, double centerX, double centerY) {
    for (int y = startY; y < endY; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            double c_re = (x - WIDTH / 2.0) * 4.0 / WIDTH / zoom + centerX;
            double c_im = (y - HEIGHT / 2.0) * 4.0 / HEIGHT / zoom + centerY;
            double z_re = c_re, z_im = c_im;
            int n;
            for (n = 0; n < MAX_ITER; ++n) {
                double z_re2 = z_re * z_re - z_im * z_im + c_re;
                z_im = 2.0 * z_re * z_im + c_im;
                z_re = z_re2;
                if (z_re * z_re + z_im * z_im > 4.0) break;
            }
            sf::Color color = mandelbrotColor(n);
            int index = (y * WIDTH + x) * 4;
            pixels[index] = color.r;
            pixels[index + 1] = color.g;
            pixels[index + 2] = color.b;
            pixels[index + 3] = 255; // Alpha
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Mandelbrot Set");
    window.setFramerateLimit(60);
    sf::Uint8* pixels = new sf::Uint8[WIDTH * HEIGHT * 4];
    sf::Texture texture;
    texture.create(WIDTH, HEIGHT);
    sf::Sprite sprite(texture);

    double zoom = 0.8;
    double centerX = -0.745;
    double centerY = 0.1;
    double targetCenterX = -0.743643887037158704752191506114774;
    double targetCenterY = 0.131825904205311970493132056385139;
    double zoomSpeed = 0.05;

    std::atomic<bool> shouldExit(false);
    std::atomic<bool> readyToRender(false);

    // Precompute initial Mandelbrot set before starting the animation
    std::thread precomputeThread([&]() {
        computeMandelbrot(pixels, 0, HEIGHT, zoom, centerX, centerY);
        texture.update(pixels);
        readyToRender = true;
        });

    precomputeThread.join();

    std::thread renderThread([&]() {
        while (!shouldExit) {
            std::vector<std::thread> threads;
            int numThreads = std::thread::hardware_concurrency();
            int rowsPerThread = HEIGHT / numThreads;
            for (int i = 0; i < numThreads; ++i) {
                int startY = i * rowsPerThread;
                int endY = (i == numThreads - 1) ? HEIGHT : (i + 1) * rowsPerThread;
                threads.emplace_back(computeMandelbrot, pixels, startY, endY, zoom, centerX, centerY);
            }
            for (auto& t : threads) {
                t.join();
            }
            texture.update(pixels);
        }
        });

    sf::Clock clock;
    float lastZoomUpdate = 0.0f;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        if (readyToRender) {
            float currentTime = clock.getElapsedTime().asSeconds();
            if (currentTime - lastZoomUpdate >= 1.0f / 60.0f) {
                lastZoomUpdate = currentTime;

                zoom *= 1.02; // Increase the zoom speed for a smoother animation
                centerX = centerX + (targetCenterX - centerX) * zoomSpeed;
                centerY = centerY + (targetCenterY - centerY) * zoomSpeed;
            }

            window.clear();
            window.draw(sprite);
            window.display();
        }
    }

    shouldExit = true;
    renderThread.join();
    delete[] pixels;
    return 0;
}
