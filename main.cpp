#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>

#include <windows.h>
#include <math.h>
#include <stdio.h>

using namespace std;

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;

int nMapWidth = 16;
int nMapHeight = 16;

float fFOV = 3.14159 / 4.0;
float fDepth = 16;

int main()
{
    // Create screen buffer
    wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    // Define the map
    wstring map;
    map += L"################";
    map += L"#..............#";
    map += L"#...########...#";
    map += L"#..............#";
    map += L"#...#......#...#";
    map += L"#..............#";
    map += L"#...#....###..##";
    map += L"#........#.....#";
    map += L"#........#.....#";
    map += L"#........#####.#";
    map += L"#..#.........#.#";
    map += L"#...#........#.#";
    map += L"#....#.......#.#";
    map += L"#.....##########";
    map += L"#..............#";
    map += L"################";

    // Gather time measurements to make movement more fluid between frames
    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();


    // Game loop
    while(true)
    {
        // Get time since last loop
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // Controls
        // Handle player rotation
        if(GetAsyncKeyState((unsigned short)'A') & 0x8000)
        {
            fPlayerA -= 1.0f * fElapsedTime;
        }
        if(GetAsyncKeyState((unsigned short)'D') & 0x8000)
        {
            fPlayerA += 1.0f * fElapsedTime;
        }

        // Handle player movement
        if(GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;

            // If the player hits a wall move them back
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        if(GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;

            // If the player hits a wall move them forwards
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }

        // Ray-tracing loop
        for(int x=0; x<nScreenWidth; x++)
        {
            // For each column, calculate the projected ray angle onto the world space
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            float fDistanceToWall = 0;
            bool bHitWall = false;
            bool bBoundary = false;

            // Unit vector for ray in player space
            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            // While this ray still has space to cover
            while(!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1f;

                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if(nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth; // Just set distance to max depth to exit loop
                }
                // Otherwise if ray is in bounds
                else
                {
                    // If ray cell is a wall block
                    if(map[nTestY * nMapWidth + nTestX] == '#')
                    {
                        bHitWall = true;

                        vector<pair<float, float>> p; // Distance to corner, Dot-product (angle between two vectors)

                        for(int tx=0; tx<2; tx++)
                        {
                            for(int ty=0; ty<2; ty++)
                            {
                                // Vector from player to perfect corner
                                float vx = (float)nTestX + tx - fPlayerX;
                                float vy = (float)nTestY + ty - fPlayerY;
                                // Magnitude of vector
                                float d = sqrt(vx*vx + vy*vy);
                                // Dot-product
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);

                                p.push_back(make_pair(d, dot));
                            }
                        }

                        // Sort pairs from closest to furthest
                        sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right)
                        {
                            return left.first < right.first;
                        });

                        float fBound = 0.005;
                        // If the angle between the two walls is less then fBound, we can assume it's the boundary of the cell
                        if(acos(p.at(0).second) < fBound)
                            bBoundary = true;
                        if(acos(p.at(1).second) < fBound)
                            bBoundary = true;
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            short wShade = ' ';
            short fShade = ' ';

            // Determine shade of wall cell based on depth
            if (fDistanceToWall <= fDepth / 4.0f)       // Very close
                wShade=0x2588;
            else if (fDistanceToWall <= fDepth / 3.0f)
                wShade=0x2593;
            else if (fDistanceToWall <= fDepth / 2.0f)
                wShade=0x2592;
            else if (fDistanceToWall <= fDepth / 1.0f)
                wShade=0x2591;
            else                                        // Very far
                wShade=' ';
            if(bBoundary)
                wShade = ' ';

            // Shade cell based on whether it's wall, ceiling or floor
            for(int y=0; y<nScreenHeight; y++)
            {
                if(y < nCeiling)
                {
                    screen[y * nScreenWidth + x] = ' ';
                }
                else if(y > nCeiling && y <= nFloor)
                {
                    screen[y * nScreenWidth + x] = wShade;
                }
                else
                {
                    // Determine shade of floor cell based on distance
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if(b<0.25)
                        fShade = '#';
                    else if(b<0.50)
                        fShade = 'x';
                    else if(b<0.75)
                        fShade = '.';
                    else if(b<0.9)
                        fShade = '-';
                    else
                        fShade = ' ';

                    screen[y * nScreenWidth + x] = fShade;
                }
            }
        }

        // Display Stats
        //swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);

        // Display Map
        for (int nx = 0; nx < nMapWidth; nx++)
        {
            for (int ny = 0; ny < nMapWidth; ny++)
            {
                screen[(ny) * nScreenWidth + nx] = map[ny * nMapWidth + (nMapWidth - nx - 1)];
            }
        }
        screen[((int)fPlayerY) * nScreenWidth + (int)(nMapWidth -  fPlayerX)] = 'P';

        // Tell console when to stop outputting string
        screen[nScreenWidth * nScreenHeight - 1] - '\0';

        // Write to screen
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, {0,0}, &dwBytesWritten);
    }

    return 0;
}
