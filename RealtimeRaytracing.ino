#include "Adafruit_Arcada.h"
Adafruit_Arcada arcada;

#define MAXSPHERECOUNT 8
#define SCREENWIDTH 160
#define SCREENHEIGHT 128
#define PIXELCOUNT (SCREENWIDTH*SCREENHEIGHT)

#define FORCEINLINE __attribute__((always_inline)) inline

float zoom = 240;
int sphereCount = 4;
int sphereLayout = 0;
int sphereLayouts = 5;
float animSpeed = 0.0025f;
float animSpeedInc = 0.000002f;
float keyPitchSpeed = 0.00075f;
float keyYawSpeed = 0.00075f;
float keyMoveSpeed = 0.006f;
float minHeight = 0.1f;
float minPitch = -1.57;
float maxPitch = 1.57f;
bool demo = false;
float demoTimePerLayout = 4 * M_PI / 2 / keyYawSpeed;

float playerPositionX = 0, playerPositionY = 2, playerPositionZ = -10;
float playerYaw = 0;
float playerPitch = -0.08f;

float lightDirectionX, lightDirectionY, lightDirectionZ;
float lightYaw = M_PI / 4;
float lightPitch = 0.64f;
float lightSize = 0.990f;

float worldUpX = 0, worldUpY = 1, worldUpZ = 0;
float playerForwardX = 0, playerForwardY = 0, playerForwardZ = 1;
float playerRightX = 1, playerRightY = 0, playerRightZ = 0;
float playerUpX = 0, playerUpY = 1, playerUpZ = 0;
float animTime = 0;
float oldAnimSpeed = 0;
float demoTime = 0;

float sphereX[MAXSPHERECOUNT], sphereY[MAXSPHERECOUNT], sphereZ[MAXSPHERECOUNT];
uint8_t mask[PIXELCOUNT];
float dither[4] = { 0.00f, 0.50f, 0.75f, 0.25f };

uint16_t imageBuffer[2][PIXELCOUNT];
int bufferIndex = 0;

void animateSpheres(uint32_t deltaTime)
{
    animTime = animTime + animSpeed * deltaTime;
    if (sphereLayout == 0)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            sphereY[i] = 1 + abs(sin(animTime + i * M_PI / sphereCount));
        }
    }
    else if (sphereLayout == 1)
    {
        float mainang = sin(animTime) * M_PI / 2;
        for (int i = 0; i < sphereCount; ++i)
        {
            float ang = mainang / 200;
            if (i == 0) ang = mainang > 0 && sphereCount > 1 ? ang : mainang;
            if (i == sphereCount-1) ang = mainang < 0 && sphereCount > 1 ? ang : mainang;
            sphereX[i] = 2 * i + 1 - sphereCount + sin(ang) * 2;
            sphereY[i] = 3.5f - cos(ang) * 2;
        }
    }
    else if (sphereLayout == 2)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            float ang = sin(animTime * (1 + i * M_PI / 8 / sphereCount)) * M_PI / 4;
            sphereY[i] = 3.5f - cos(ang) * 2;
            sphereZ[i] = sin(ang) * 2;
        }
    }
    else if (sphereLayout == 3)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            float ang = fmodf(fmodf(animTime / sphereCount + i * M_PI / sphereCount, M_PI) + M_PI, M_PI);
            float r = (sphereCount == 1) ? 1 : 0.25f + 1 / sin(M_PI / 2 / sphereCount);
            sphereX[i] = r * cos(ang);
            sphereY[i] = r * sin(ang) - 1;
        }
    }
    else if (sphereLayout == 4)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            float ang1 = i * 2 * M_PI / sphereCount;
            float ang2 = 2 * animTime / sphereCount;
            float r = (sphereCount == 1) ? 0 : 1 / sin(M_PI / sphereCount);
            sphereX[i] = sin(ang1 + ang2) * r + cos(ang2) * r;
            sphereZ[i] = cos(ang1 + ang2) * r + sin(ang2) * r;
        }
    }
}

void createSpheres()
{
    if (sphereLayout == 0)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            float r = (sphereCount == 1) ? 0 : 0.5 + 1 / sin(M_PI / sphereCount);
            sphereX[i] = r * sin(M_PI / 4 + i * 2 * M_PI / sphereCount);
            sphereZ[i] = r * cos(M_PI / 4 + i * 2 * M_PI / sphereCount);
        }
    }
    else if (sphereLayout == 1)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            sphereZ[i] = 0;
        }
    }
    else if (sphereLayout == 2)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            sphereX[i] = 2 * i + 1 - sphereCount;
        }
    }
    else if (sphereLayout == 3)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            sphereZ[i] = 0;
        }
    }
    else if (sphereLayout == 4)
    {
        for (int i = 0; i < sphereCount; ++i)
        {
            sphereY[i] = 1;
        }
    }
    animateSpheres(0);
}

void calculate2DData()
{
    memset(mask, 0, sizeof(mask));

    for (int i = 0; i < sphereCount; ++i)
    {
        int sphereMask = 1 << i;
        float dx = sphereX[i] - playerPositionX;
        float dy = sphereY[i] - playerPositionY;
        float dz = sphereZ[i] - playerPositionZ;
        float dotRight = dx * playerRightX + dy * playerRightY + dz * playerRightZ;
        float dotUp = dx * playerUpX + dy * playerUpY + dz * playerUpZ;
        float dotForward = dx * playerForwardX + dy * playerForwardY + dz * playerForwardZ;
        if (dotForward > 0.5f)
        {
            int screenX = ((dotRight / dotForward) * zoom + SCREENWIDTH/2);
            int screenY = ((-dotUp / dotForward) * zoom + SCREENHEIGHT/2);
            int radius = (zoom / (dotForward - 0.5f) + 4);

            int startY = screenY + 1 - radius;
            int endY = screenY - 1 + radius;
            int r2 = radius * radius;
            if (startY < 0) startY = 0;
            if (endY >= SCREENHEIGHT) endY = SCREENHEIGHT-1;
            for (int y = startY; y <= endY; ++y)
            {
                dy = y - screenY;
                dx = sqrt(r2 - dy * dy);
                int startX = screenX - dx;
                int endX = screenX + dx;
                if (startX < 0) startX = 0;
                if (endX >= SCREENWIDTH) endX = SCREENWIDTH-1;
                int startIndex = y*SCREENWIDTH+startX;
                int endIndex = startIndex-startX+endX;
                for (int maskIndex = startIndex; maskIndex <= endIndex; ++maskIndex)
                {
                    mask[maskIndex] |= sphereMask;
                }
            }
        }
        else
        {
            if (dx*dx + dy*dy + dz*dz < 2.25f)
            {
                for (int i = 0; i < PIXELCOUNT; ++i)
                {
                    mask[i] |= sphereMask;
                }
            }
        }
    }
}

bool startButton = false;
bool selectButton = false;
void Controls(uint32_t deltaTime)
{
    uint8_t heldButtons = arcada.readButtons();
    uint8_t pressedButtons = arcada.justPressedButtons();
    uint8_t releasedButtons = arcada.justReleasedButtons();

    if (pressedButtons & ARCADA_BUTTONMASK_START)
    {
        startButton = true;
    }
    if (startButton && (releasedButtons & ARCADA_BUTTONMASK_START))
    {
        if (animSpeed == 0) { animSpeed = oldAnimSpeed; } else { oldAnimSpeed = animSpeed; animSpeed = 0; }
    }

    if (pressedButtons & ARCADA_BUTTONMASK_SELECT)
    {
        selectButton = true;
    }
    if (selectButton && (releasedButtons & ARCADA_BUTTONMASK_SELECT))
    {
        if (!demo)
        {
            sphereLayout = (sphereLayout + 1) % sphereLayouts; animTime = 0; createSpheres();
        }
    }

    if (heldButtons & ARCADA_BUTTONMASK_START)
    {
        if (heldButtons & ~ARCADA_BUTTONMASK_START) { startButton = false; }
        if (heldButtons & ARCADA_BUTTONMASK_A) { animSpeed = animSpeed + animSpeedInc * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_B) { animSpeed = animSpeed - animSpeedInc * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_UP) { lightPitch += keyPitchSpeed * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_DOWN) { lightPitch -= keyPitchSpeed * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_LEFT) { lightYaw += keyPitchSpeed * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_RIGHT) { lightYaw -= keyPitchSpeed * deltaTime; }
    }
    else if (heldButtons & ARCADA_BUTTONMASK_SELECT)
    {
        if (heldButtons & ~ARCADA_BUTTONMASK_SELECT) { selectButton = false; }
        if (releasedButtons & ARCADA_BUTTONMASK_A) { sphereCount = sphereCount < MAXSPHERECOUNT ? sphereCount + 1 : MAXSPHERECOUNT; createSpheres(); }
        if (releasedButtons & ARCADA_BUTTONMASK_B) { sphereCount = sphereCount > 1 ? sphereCount - 1 : 1; createSpheres(); }
    }
    else if ((heldButtons & ARCADA_BUTTONMASK_A) && (heldButtons & ARCADA_BUTTONMASK_B))
    {
        if (!demo)
        {
            if (heldButtons & ARCADA_BUTTONMASK_UP)
            {
                playerPositionX += playerUpX * deltaTime * keyMoveSpeed;
                playerPositionY += playerUpY * deltaTime * keyMoveSpeed;
                playerPositionZ += playerUpZ * deltaTime * keyMoveSpeed;
            }
            if (heldButtons & ARCADA_BUTTONMASK_DOWN)
            {
                playerPositionX -= playerUpX * deltaTime * keyMoveSpeed;
                playerPositionY -= playerUpY * deltaTime * keyMoveSpeed;
                playerPositionZ -= playerUpZ * deltaTime * keyMoveSpeed;
            }
            if (heldButtons & ARCADA_BUTTONMASK_LEFT)
            {
                playerPositionX -= playerRightX * deltaTime * keyMoveSpeed;
                playerPositionY -= playerRightY * deltaTime * keyMoveSpeed;
                playerPositionZ -= playerRightZ * deltaTime * keyMoveSpeed;
            }
            if (heldButtons & ARCADA_BUTTONMASK_RIGHT)
            {
                playerPositionX += playerRightX * deltaTime * keyMoveSpeed;
                playerPositionY += playerRightY * deltaTime * keyMoveSpeed;
                playerPositionZ += playerRightZ * deltaTime * keyMoveSpeed;
            }
        }
    }
    else if (!demo)
    {
        if (heldButtons & ARCADA_BUTTONMASK_UP) { playerPitch += keyPitchSpeed * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_DOWN) { playerPitch -= keyPitchSpeed * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_LEFT) { playerYaw += keyYawSpeed * deltaTime; }
        if (heldButtons & ARCADA_BUTTONMASK_RIGHT) { playerYaw -= keyYawSpeed * deltaTime; }

        if (playerPitch > maxPitch) { playerPitch = maxPitch; }
        if (playerPitch < minPitch) { playerPitch = minPitch; }

        if (heldButtons & ARCADA_BUTTONMASK_A)
        {
            playerPositionX += playerForwardX * deltaTime * keyMoveSpeed;
            playerPositionY += playerForwardY * deltaTime * keyMoveSpeed;
            playerPositionZ += playerForwardZ * deltaTime * keyMoveSpeed;
        }
        if (heldButtons & ARCADA_BUTTONMASK_B)
        {
            playerPositionX -= playerForwardX * deltaTime * keyMoveSpeed;
            playerPositionY -= playerForwardY * deltaTime * keyMoveSpeed;
            playerPositionZ -= playerForwardZ * deltaTime * keyMoveSpeed;
        }
    }

    if (playerPositionY < minHeight) { playerPositionY = minHeight; }

    float cosPitch = cosf(playerPitch);
    playerForwardX = sinf(playerYaw) * cosPitch;
    playerForwardY = sinf(playerPitch);
    playerForwardZ = cosf(playerYaw) * cosPitch;
    playerRightX = playerForwardY * worldUpZ - playerForwardZ * worldUpY;
    playerRightY = playerForwardZ * worldUpX - playerForwardX * worldUpZ;
    playerRightZ = playerForwardX * worldUpY - playerForwardY * worldUpX;
    float length = sqrtf(playerRightX * playerRightX + playerRightY * playerRightY + playerRightZ * playerRightZ);
    playerRightX /= length;
    playerRightY /= length;
    playerRightZ /= length;
    playerUpX = playerRightY * playerForwardZ - playerRightZ * playerForwardY;
    playerUpY = playerRightZ * playerForwardX - playerRightX * playerForwardZ;
    playerUpZ = playerRightX * playerForwardY - playerRightY * playerForwardX;

    cosPitch = cosf(lightPitch);
    lightDirectionX = sinf(lightYaw) * cosPitch;
    lightDirectionY = sinf(lightPitch);
    lightDirectionZ = cosf(lightYaw) * cosPitch;

    if ((heldButtons & ARCADA_BUTTONMASK_START) && (heldButtons & ARCADA_BUTTONMASK_SELECT) && ((pressedButtons & ARCADA_BUTTONMASK_START) || (pressedButtons & ARCADA_BUTTONMASK_SELECT)))
    {
        startButton = false; selectButton = false;
        demo = !demo;
        demoTime = 0;
    }

    if (demo)
    {
        playerPositionX = -10 * sin(playerYaw);
        playerPositionZ = -10 * cos(playerYaw);
        playerYaw += keyYawSpeed * deltaTime;
        playerPitch = -0.08f;

        int newSphereLayout = (int)(demoTime / demoTimePerLayout) % sphereLayouts;
        if (newSphereLayout != sphereLayout) { sphereLayout = newSphereLayout; animTime = 0; createSpheres(); }
        demoTime += deltaTime;
    }
}

FORCEINLINE int FastFloor(float x)
{
    return (x < 0) ? ((int)x - 1) : (int)x;
}

void Blit(uint16_t *ptr)
{
    arcada.display->dmaWait();
    arcada.display->endWrite();
    arcada.display->startWrite();
    arcada.display->setAddrWindow(0, 0, SCREENWIDTH, SCREENHEIGHT);
    arcada.display->writePixels(ptr, PIXELCOUNT, false, true);
}

void setup(void)
{
    Serial.begin(9600);

    arcada.arcadaBegin();
    arcada.displayBegin();
    arcada.display->fillScreen(0);
    delay(16);
    arcada.setBacklight(255);

    createSpheres();
}

uint32_t oldTime = 0;
void loop()
{
    uint32_t time = millis();
    uint32_t deltaTime = time - oldTime;
    oldTime = time;


    Controls(deltaTime);
    animateSpheres(deltaTime);
    calculate2DData();

    float rayRightStepX = playerRightX / zoom; float rayRightStepY = playerRightY / zoom; float rayRightStepZ = playerRightZ / zoom;
    float rayUpStepX = playerUpX / zoom; float rayUpStepY = playerUpY / zoom; float rayUpStepZ = playerUpZ / zoom;
    float rayLineJumpX = -rayUpStepX - SCREENWIDTH * rayRightStepX;
    float rayLineJumpY = -rayUpStepY - SCREENWIDTH * rayRightStepY;
    float rayLineJumpZ = -rayUpStepZ - SCREENWIDTH * rayRightStepZ;
    float rayDirectionX = playerForwardX - rayRightStepX * SCREENWIDTH/2 + rayUpStepX * SCREENHEIGHT/2;
    float rayDirectionY = playerForwardY - rayRightStepY * SCREENWIDTH/2 + rayUpStepY * SCREENHEIGHT/2;
    float rayDirectionZ = playerForwardZ - rayRightStepZ * SCREENWIDTH/2 + rayUpStepZ * SCREENHEIGHT/2;
    float rayPositionX, rayPositionY, rayPositionZ, rayDirX, rayDirY, rayDirZ, lX, lY, lZ, tc, d2, t1;
    float u, normalX, normalY, normalZ, doubledot, sphX, sphY, sphZ, closestSphX, closestSphY, closestSphZ, closestDistance, length;
    uint8_t maskValue;
    uint16_t colour;
    int bounce;

    uint8_t* maskPtr = mask;
    uint16_t* bufferPtr = imageBuffer[bufferIndex];
    for (int y = 0; y < SCREENHEIGHT; ++y)
    {
        for (int x = 0; x < SCREENWIDTH; ++x)
        {
            rayPositionX = playerPositionX; rayPositionY = playerPositionY; rayPositionZ = playerPositionZ;
            length = sqrt(rayDirectionX * rayDirectionX + rayDirectionY * rayDirectionY + rayDirectionZ * rayDirectionZ);
            rayDirX = rayDirectionX / length; rayDirY = rayDirectionY / length; rayDirZ = rayDirectionZ / length;

            bounce = 0;
            maskValue = *maskPtr++;
            if (maskValue > 0)
            {
                while (bounce < 4)
                {
                    closestDistance = 1e37f;
                    for (int i = 0; i < sphereCount; ++i)
                    {
                        if (bounce > 0 || (maskValue & (1 << i)) != 0)
                        {
                            sphX = sphereX[i]; sphY = sphereY[i]; sphZ = sphereZ[i];
                            lX = sphX - rayPositionX; lY = sphY - rayPositionY; lZ = sphZ - rayPositionZ;
                            tc = lX * rayDirX + lY * rayDirY + lZ * rayDirZ;
                            if (tc > 0)
                            {
                                d2 = lX * lX + lY * lY + lZ * lZ - tc * tc;
                                if (d2 < 1)
                                {
                                    t1 = tc - sqrtf(1 - d2);
                                    if (t1 < closestDistance && t1 > 0)
                                    {
                                        closestDistance = t1;
                                        closestSphX = sphX; closestSphY = sphY; closestSphZ = sphZ;
                                    }
                                }
                            }
                        }
                    }
                    if (closestDistance == 1e37f)
                    {
                        break;
                    }
                    else
                    {
                        rayPositionX += rayDirX * closestDistance;
                        rayPositionY += rayDirY * closestDistance;
                        rayPositionZ += rayDirZ * closestDistance;
                        if (rayPositionY < 0)
                        {
                            break;
                        }
                        normalX = rayPositionX - closestSphX;
                        normalY = rayPositionY - closestSphY;
                        normalZ = rayPositionZ - closestSphZ;
                        doubledot = (normalX * rayDirX + normalY * rayDirY + normalZ * rayDirZ) * 2;
                        rayDirX -= doubledot * normalX;
                        rayDirY -= doubledot * normalY;
                        rayDirZ -= doubledot * normalZ;
                    }
                    bounce = bounce + 1;
                }
            }

            if (rayDirY >= 0)
            {
                if (rayDirX * lightDirectionX + rayDirY * lightDirectionY + rayDirZ * lightDirectionZ > lightSize && bounce < 3)
                {
                    colour = 0xFFF8;
                }
                else
                {
                    colour = ((int)((0.25f + 0.75f * rayDirY) * (31-bounce*4) + dither[(x&1)+(y&1)*2])) * 0x21;
                }
            }
            else
            {
                u = rayPositionY / rayDirY;
                rayPositionX = rayPositionX - u * rayDirX;
                rayPositionZ = rayPositionZ - u * rayDirZ;
                for (int i = 0; i < sphereCount; ++i)
                {
                    lX = sphereX[i] - rayPositionX;
                    lY = sphereY[i];
                    lZ = sphereZ[i] - rayPositionZ;
                    tc = lX * lightDirectionX + lY * lightDirectionY + lZ * lightDirectionZ;
                    if (tc > 0 && lX * lX + lY * lY + lZ * lZ - tc * tc < 1 && lightDirectionY > 0)
                    {
                        bounce = bounce + 1;
                        break;
                    }
                }
                colour = (31-(((FastFloor(rayPositionX) + FastFloor(rayPositionZ)) & 1) + bounce)*4) * 0x20;
            }
            *bufferPtr++ = (colour >> 8) | (colour << 8);
            rayDirectionX = rayDirectionX + rayRightStepX;
            rayDirectionY = rayDirectionY + rayRightStepY;
            rayDirectionZ = rayDirectionZ + rayRightStepZ;
        }
        rayDirectionX = rayDirectionX + rayLineJumpX;
        rayDirectionY = rayDirectionY + rayLineJumpY;
        rayDirectionZ = rayDirectionZ + rayLineJumpZ;
    }


    Blit(imageBuffer[bufferIndex]);
    bufferIndex = !bufferIndex;

    Serial.println(deltaTime);
}
