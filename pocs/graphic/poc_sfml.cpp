#include <SFML/Graphics.hpp>

int main()
{
    // Create window 800x600
    sf::RenderWindow window(sf::VideoMode(800, 600), "POC SFML - Red Circle");
    window.setFramerateLimit(60);

    // Create red circle in the center
    sf::CircleShape circle(50.f);
    circle.setFillColor(sf::Color::Red);
    circle.setPosition(400.f - 50.f, 300.f - 50.f); // Center position (minus radius)

    // Main loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);
        window.draw(circle);
        window.display();
    }

    return 0;
}
