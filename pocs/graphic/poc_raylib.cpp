#include <raylib.h>

int main()
{
    // Initialize window 800x600
    InitWindow(800, 600, "POC Raylib - Red Circle");
    SetTargetFPS(60);

    // Main loop
    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        // Draw red circle in the center (x, y, radius, color)
        DrawCircle(400, 300, 50, RED);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
