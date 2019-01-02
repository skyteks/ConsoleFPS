/*
	OneLoneCoder.com - Command Line First Person Shooter (FPS) Engine
	"Why were games not done like this is 1990?" - @Javidx9

	License
	~~~~~~~
	Copyright (C) 2018  Javidx9
	This program comes with ABSOLUTELY NO WARRANTY.
	This is free software, and you are welcome to redistribute it
	under certain conditions; See license for details.
	Original works located at:
	https://www.github.com/onelonecoder
	https://www.onelonecoder.com
	https://www.youtube.com/javidx9

	GNU GPLv3
	https://github.com/OneLoneCoder/videos/blob/master/LICENSE
*/

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <math.h>
using namespace std;

#include <stdio.h>
#include <Windows.h>

int nScreenWidth = 120;// Console Screen Size X (columns)
int nScreenHeight = 40;// Console Screen Size Y (rows)

float fPlayerX = 14.7f;// Player Start Position
float fPlayerY = 5.09f;
float fPlayerA = 0.0f;// Player Start Rotation

int nMapHeight = 16;// World Dimensions
int nMapWidth = 16;

const float fPi = 3.14159f;
float fFOV = fPi / 4.0f;// Field of View
float fDepth = 16.0f;// Maximum rendering distance
float fSpeed = 5.0f;// Walking Speed
int main()
{
	// Create Screen Buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight + 1];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;
	map += L"#########......#";
	map += L"#..............#";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// Game loop
	while (1)//for (;;)
	{
		// We'll need time differential per frame to calculate modification
		// to movement speeds, to ensure consistant movement, as ray-tracing
		// is non-deterministic
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Controls
		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
		{
			fPlayerA -= (fSpeed * 0.4f) * fElapsedTime;
			while (fPlayerA > fPi)
			{
				fPlayerA -= fPi * 2.0f;
			}
			while (fPlayerA < -fPi)
			{
				fPlayerA += fPi * 2.0f;
			}
		}
		// Handle CW Rotation
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
		{
			fPlayerA += (fSpeed * 0.4f) * fElapsedTime;
			while (fPlayerA > fPi)
			{
				fPlayerA -= fPi * 2.0f;
			}
			while (fPlayerA < -fPi)
			{
				fPlayerA += fPi * 2.0f;
			}
		}
		// Handle Forwards movement & collision
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;

			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
			{
				fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
			}

			fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;

			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
			{
				fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
			}
		}
		// Handle backwards movement & collision
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;

			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
			{
				fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
			}

			fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;

			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
			{
				fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; x++)
		{
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
			// Find distance to wall
			float fStepSize = 0.1f;// Increment size for ray casting, decrease to increase
			float fDistanceToWall = 0.0f;// Resolution
			bool bHitWall = false;// Set when ray hits wall block
			bool bBoundary = false;// Set when ray hits boundary between two wall blocks

			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			// Incrementally cast ray from player, along ray angle, testing for 
			// intersection with a block
			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += fStepSize;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth;// Just set the distance to maximum depth
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map.c_str()[nTestX * nMapWidth + nTestY] == '#')
					{
						// Ray has hit wall
						bHitWall = true;

						// To highlight tile boundaries, cast a ray from each corner
						// of the tile, to the player. The more coincident this ray
						// is to the rendering ray, the closer we are to a tile 
						// boundary, which we'll shade to add detail to the walls
						vector<pair<float, float>> p; // distance, dot

						// Test each corner of hit tile, storing the distance from
						// the player, and the calculated dot product of the two rays
						for (int tx = 0; tx < 2; tx++)
						{
							for (int ty = 0; ty < 2; ty++)
							{
								// Angle of corner to eye
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + ty - fPlayerY;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}
						// Sort pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair <float, float> &left, const pair<float, float> &right) {return left.first < right.first; });

						// First two/three are closest (we will never see all four)
						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';
			for (int y = 0; y < nScreenHeight; y++)
			{
				// Each Row
				if (y <= nCeiling)
				{
					screen[y * nScreenWidth + x] = ' ';
				}
				else if (y > nCeiling && y <= nFloor)
				{
					if (fDistanceToWall <= fDepth / 4.0f) nShade = 0x2588;
					else if (fDistanceToWall < fDepth / 3.0f) nShade = 0x2593;
					else if (fDistanceToWall < fDepth / 2.0f) nShade = 0x2592;
					else if (fDistanceToWall < fDepth) nShade = 0x2591;
					else nShade = ' ';

					//if (bBoundary) nShade = 'I';

					screen[y * nScreenWidth + x] = nShade;
				}
				else
				{
					// Shade floor based on distance
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25f) nShade = '#';
					else if (b < 0.5f) nShade = 'x';
					else if (b < 0.75) nShade = '-';
					else if (b < 0.9) nShade = '.';
					else nShade = ' ';

					screen[y * nScreenWidth + x] = nShade;
				}
			}
		}

		// Display stats
		swprintf_s(screen, nScreenHeight, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, (1.0f / fElapsedTime));

		// Display map
		for (int nx = 0; nx < nMapWidth; nx++)
		{
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}

		// Display player in map
		short nPlayer = 'P';
		if (fPlayerA > (-fPi / 4.0f) && fPlayerA < (fPi / 4.0f))
		{
			nPlayer = 0x2192;// right
		}
		else if (fPlayerA > (-fPi / 4.0f) * 3.0f && fPlayerA < (-fPi / 4.0f))
		{
			nPlayer = 0x2191;// up
		}
		else if (fPlayerA > (fPi / 4.0f) && fPlayerA < (fPi / 4.0f) * 3.0f)
		{
			nPlayer = 0x2193;// down
		}
		else
		{
			nPlayer = 0x2190;// left
		}
		screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = nPlayer;

		screen[nScreenWidth * nScreenHeight] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}