#include <GL/glut.h>
#include <string>
#include <cmath>

// Scenes
bool showTitle = true;
bool showThinking = false;
bool showNext = false; // placeholder for next scenes
bool showKitchenReturn = false;
bool showPlateScene = false;
bool waitingForEnter = false;



// animation globals
bool animateBubble = true;
float bubblePhase = 0.0f;      // for bobbing
float liftPhase = 0.0f;        // for mini-Kevin arm animation
float chandelierPhase = 0.0f;  // for chandelier swing
float fireworksPhase = 0.0f;   // NEW: for simple next-scene fireworks
float kevinX = -0.8f;    // start far left
int movePhase = 0;       // 0 = entering, 1 = waiting, 2 = exiting

// NEW: clock / midnight & fireworks state
bool clockAtMidnight = false;   // false -> shows 11:59; true -> shows 12:00 AM
bool fireworksActive = false;   // when true, draw fireworks & Happy New Year
// rectangle of the clock in world coords (updated by drawDigitalClock)
float clockLeft = 0.0f, clockTop = 0.0f, clockRight = 0.0f, clockBottom = 0.0f;

// Kevin walking (new)
bool kevinWalking = false;
float kevinPosX = -1.2f;    // current x (will be initialized when scene starts)
float kevinPosY = -0.38f;   // fixed Y where Kevin stands
float kevinTargetX = 0.0f;  // where Kevin should stop (middle)
float kevinSpeed = 0.012f;  // movement per frame

// fridge footprint (exposed so mouse/click code can use same coords)
float fridgeRX1 = 0.58f, fridgeRY1 = -0.95f, fridgeRX2 = 1.02f, fridgeRY2 = 0.75f;

// Kevin action state for interactive sequence
enum KevinAction { K_IDLE = 0, K_WALK_TO_FRIDGE, K_PICK_DRINK, K_WALK_BACK, K_DRINKING };
KevinAction kevinAction = K_IDLE;
int pickTimer = 0;
int drinkTimer = 0;
bool kevinHasDrink = false;

// NEW: track initial walk so bubble appears only after Kevin reaches middle
bool initialWalkStarted = false;
bool initialWalkDone = false;

// NEW: view extents used to fill the full window
float viewLeft = -1.0f, viewRight = 1.0f, viewBottom = -1.0f, viewTop = 1.0f;

// Basic helpers
void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
}

void drawRect(float x1, float y1, float x2, float y2) {
    glBegin(GL_POLYGON);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void drawCircle(float cx, float cy, float r, int segments = 36) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

// small helpers for night sky (glow implemented)
void drawStar(float x, float y, float s) {
    // soft glow (additive)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 1.0f, 0.92f, 0.18f);
    drawCircle(x, y, s * 3.5f, 16);
    glColor4f(1.0f, 1.0f, 0.92f, 0.08f);
    drawCircle(x, y, s * 6.0f, 12);

    // core star
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(1.0f, 1.0f, 0.92f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    int spikes = 5;
    for (int i = 0; i <= spikes * 2; ++i) {
        float a = i * 3.1415926f / spikes;
        float r = (i % 2 == 0) ? s : s * 0.4f;
        glVertex2f(x + cosf(a) * r, y + sinf(a) * r);
    }
    glEnd();
    glDisable(GL_BLEND);
}

void drawMoon(float x, float y, float r) {
    // glow halo (additive)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.98f, 0.97f, 0.88f, 0.24f);
    drawCircle(x, y, r * 1.8f, 32);
    glColor4f(0.98f, 0.97f, 0.88f, 0.12f);
    drawCircle(x, y, r * 1.25f, 24);

    // moon core and crescent (normal blend)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(0.98f, 0.97f, 0.88f);
    drawCircle(x, y, r, 32);
    // crescent carving
    glColor3f(0.05f, 0.08f, 0.20f);
    drawCircle(x + r * 0.35f, y + r * 0.12f, r * 0.78f, 32);

    glDisable(GL_BLEND);
}

// Kevin (same simple style)
void drawKevin(float x, float y, float scale = 1.0f) {
    // hair
    glColor3f(0.98f, 0.80f, 0.42f);
    drawCircle(x, y + 0.28f * scale, 0.13f * scale);
    // head
    glColor3f(1.0f, 0.8f, 0.6f);
    drawCircle(x, y + 0.25f * scale, 0.12f * scale);

    // eyes
    float ex = 0.03f * scale, ey = 0.03f * scale;
    float er = 0.03f * scale, pr = 0.005f * scale;
    glColor3f(1, 1, 1); drawCircle(x - ex, y + 0.25f * scale + ey, er);
    glColor3f(0, 0, 0); drawCircle(x - ex, y + 0.25f * scale + ey, pr);
    glColor3f(1, 1, 1); drawCircle(x + ex, y + 0.25f * scale + ey, er);
    glColor3f(0, 0, 0); drawCircle(x + ex, y + 0.25f * scale + ey, pr);

    // mouth
    glColor3f(0.6f, 0.0f, 0.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x - 0.03f * scale, y + 0.22f * scale);
    glVertex2f(x, y + 0.20f * scale);
    glVertex2f(x + 0.03f * scale, y + 0.22f * scale);
    glEnd();

    // torso
    glColor3f(0.2f, 0.3f, 0.8f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.1f * scale, y);
    glVertex2f(x + 0.1f * scale, y);
    glVertex2f(x + 0.1f * scale, y + 0.2f * scale);
    glVertex2f(x - 0.1f * scale, y + 0.2f * scale);
    glEnd();

    // arms (both down)
    glColor3f(1.0f, 0.8f, 0.6f);
    // left down
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.12f * scale, y + 0.18f * scale);
    glVertex2f(x - 0.10f * scale, y + 0.18f * scale);
    glVertex2f(x - 0.10f * scale, y + 0.05f * scale);
    glVertex2f(x - 0.12f * scale, y + 0.05f * scale);
    glEnd();
    // right down (mirrored)
    glBegin(GL_POLYGON);
    glVertex2f(x + 0.10f * scale, y + 0.18f * scale);
    glVertex2f(x + 0.12f * scale, y + 0.18f * scale);
    glVertex2f(x + 0.12f * scale, y + 0.05f * scale);
    glVertex2f(x + 0.10f * scale, y + 0.05f * scale);
    glEnd();

    // legs
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.08f * scale, y - 0.1f * scale);
    glVertex2f(x - 0.02f * scale, y - 0.1f * scale);
    glVertex2f(x - 0.02f * scale, y);
    glVertex2f(x - 0.08f * scale, y);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2f(x + 0.02f * scale, y - 0.1f * scale);
    glVertex2f(x + 0.08f * scale, y - 0.1f * scale);
    glVertex2f(x + 0.08f * scale, y);
    glVertex2f(x + 0.02f * scale, y);
    glEnd();
}

// small icons for thought bubble
void drawDumbbellSmall(float x, float y, float s = 0.05f) {
    // bar
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_POLYGON);
    glVertex2f(x - s, y - 0.005f);
    glVertex2f(x + s, y - 0.005f);
    glVertex2f(x + s, y + 0.005f);
    glVertex2f(x - s, y + 0.005f);
    glEnd();
    // left weight
    glColor3f(0.15f, 0.15f, 0.15f);
    drawCircle(x - s - 0.02f, y, 0.02f);
    // right weight
    drawCircle(x + s + 0.02f, y, 0.02f);
}

// Mini-Kevin lifting animation inside bubble
void drawMiniKevinLift(float cx, float cy, float scale, float t) {
    // t in radians -> controls arm lift (-1..1)
    float armLift = (sinf(t) + 1.0f) * 0.5f; // 0..1
    float armOffset = armLift * 0.06f * scale;

    // head
    glColor3f(0.98f, 0.80f, 0.42f);
    drawCircle(cx, cy + 0.09f * scale, 0.045f * scale); // hair
    glColor3f(1.0f, 0.8f, 0.6f);
    drawCircle(cx, cy + 0.08f * scale, 0.04f * scale); // head

    // eyes
    glColor3f(1, 1, 1); drawCircle(cx - 0.01f * scale, cy + 0.085f * scale, 0.01f * scale);
    glColor3f(0, 0, 0); drawCircle(cx - 0.01f * scale, cy + 0.085f * scale, 0.004f * scale);
    glColor3f(1, 1, 1); drawCircle(cx + 0.01f * scale, cy + 0.085f * scale, 0.01f * scale);
    glColor3f(0, 0, 0); drawCircle(cx + 0.01f * scale, cy + 0.085f * scale, 0.004f * scale);

    // torso
    glColor3f(0.2f, 0.3f, 0.8f);
    glBegin(GL_POLYGON);
    glVertex2f(cx - 0.03f * scale, cy - 0.01f * scale);
    glVertex2f(cx + 0.03f * scale, cy - 0.01f * scale);
    glVertex2f(cx + 0.03f * scale, cy + 0.05f * scale);
    glVertex2f(cx - 0.03f * scale, cy + 0.05f * scale);
    glEnd();

    // arms - left and right raise together
    glColor3f(1.0f, 0.8f, 0.6f);
    // left arm
    glBegin(GL_POLYGON);
    glVertex2f(cx - 0.035f * scale, cy + 0.035f * scale);
    glVertex2f(cx - 0.015f * scale, cy + 0.035f * scale);
    glVertex2f(cx - 0.015f * scale, cy + 0.035f * scale + armOffset);
    glVertex2f(cx - 0.035f * scale, cy + 0.035f * scale + armOffset);
    glEnd();
    // right arm
    glBegin(GL_POLYGON);
    glVertex2f(cx + 0.015f * scale, cy + 0.035f * scale);
    glVertex2f(cx + 0.035f * scale, cy + 0.035f * scale);
    glVertex2f(cx + 0.035f * scale, cy + 0.035f * scale + armOffset);
    glVertex2f(cx + 0.015f * scale, cy + 0.035f * scale + armOffset);
    glEnd();

    // dumbbell follows arms (centered)
    glPushMatrix();
    float dbY = cy + 0.035f * scale + armOffset + 0.01f * scale;
    drawDumbbellSmall(cx, dbY, 0.035f * scale);
    glPopMatrix();
}

// Thought bubble helper
void drawThoughtBubble(float cx, float cy, float r) {
    glColor3f(1, 1, 1);
    drawCircle(cx, cy, r, 48);
    drawCircle(cx - r * 0.6f, cy - r * 0.6f, r * 0.18f);
    drawCircle(cx - r * 0.85f, cy - r * 0.85f, r * 0.08f);
    glColor3f(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    for (int i = 0;i < 48;i++) {
        float theta = 2.0f * 3.1415926f * float(i) / 48.0f;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

// NEW: chandelier drawing (swing uses phase)
void drawChandelier(float cx, float cy, float scale, float phase) {
    float angleDeg = sinf(phase) * 8.0f; // swing amount
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);

    // chain
    glColor3f(0.7f, 0.6f, 0.4f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.06f * scale);
    glVertex2f(0.0f, -0.02f * scale);
    glEnd();

    // central body
    glColor3f(0.85f, 0.75f, 0.5f);
    drawCircle(0.0f, -0.04f * scale, 0.05f * scale, 24);

    // arms and bulbs (6 arms)
    int arms = 6;
    for (int i = 0; i < arms; ++i) {
        float a = 2.0f * 3.1415926f * i / arms;
        float ax = cosf(a);
        float ay = sinf(a);
        float len = 0.18f * scale;
        // arm
        glColor3f(0.75f, 0.65f, 0.45f);
        glBegin(GL_LINES);
        glVertex2f(0.0f, -0.04f * scale);
        glVertex2f(ax * len, -0.04f * scale + ay * len);
        glEnd();
        // bulb / candle
        float bx = ax * len;
        float by = -0.04f * scale + ay * len;
        glColor3f(1.0f, 0.95f, 0.6f);
        drawCircle(bx, by, 0.03f * scale, 16);
        // small glow
        glColor3f(1.0f, 0.9f, 0.5f);
        drawCircle(bx, by, 0.015f * scale, 12);
    }

    glPopMatrix();
}

// NEW: food items on table
void drawPlateWithFood(float px, float py, float scale = 1.0f) {
    // plate
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(px, py, 0.09f * scale, 24);
    glColor3f(0.9f, 0.9f, 0.95f);
    drawCircle(px, py, 0.08f * scale, 24);

    // food blobs
    glColor3f(0.9f, 0.5f, 0.2f);
    drawCircle(px - 0.01f * scale, py - 0.005f * scale, 0.05f * scale);
    glColor3f(0.8f, 0.3f, 0.1f);
    drawCircle(px + 0.03f * scale, py + 0.005f * scale, 0.03f * scale);
}

void drawRoastChicken(float x, float y, float s = 0.08f) {
    // main body
    glColor3f(0.85f, 0.45f, 0.15f);
    drawCircle(x, y, s, 20);

    // off-center contour/bulge
    glColor3f(0.75f, 0.35f, 0.10f);

    // Increase offset so it’s visibly off-center
    float offsetX = -0.9f * s;   // move left
    float offsetY = +0.18f * s;   // move slightly upward

    drawCircle(x + offsetX, y + offsetY, s * 0.45f, 16);
}


void drawCake(float x, float y, float s = 0.07f) {

    // icing
    glColor3f(1.0f, 0.4f, 0.7f);
    drawCircle(x, y + 0.12f * s, s * 0.6f, 18);
    // candle
    glColor3f(1, 0.8f, 0.2f);
    glBegin(GL_LINES);
    glVertex2f(x, y + s * 0.38f);
    glVertex2f(x, y + s * 0.58f);
    glEnd();
    glColor3f(1, 0.6f, 0.1f);
    drawCircle(x, y + s * 0.6f, s * 0.05f, 8);
}

void drawGlass(float x, float y, float h = 0.08f) {
    glColor3f(0.85f, 0.95f, 1.0f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.02f, y - 0.02f);
    glVertex2f(x + 0.02f, y - 0.02f);
    glVertex2f(x + 0.03f, y + h - 0.02f);
    glVertex2f(x - 0.03f, y + h - 0.02f);
    glEnd();
    glColor3f(0.2f, 0.6f, 1.0f);
    drawCircle(x, y + h - 0.02f, 0.015f);
}

// NEW: table with many foods
void drawDiningTable(float centerX, float topY, float width = 1.6f, float depth = 0.28f) {

    float dx = -0.5f;   // shift whole table left

    // tabletop
    glColor3f(0.55f, 0.33f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex2f(centerX + dx - width / 2.0f, topY);
    glVertex2f(centerX + dx + width / 2.0f, topY);
    glVertex2f(centerX + dx + width / 2.0f, topY - depth);
    glVertex2f(centerX + dx - width / 2.0f, topY - depth);
    glEnd();

    // legs
    float legW = 0.04f, legH = 0.15f;
    glColor3f(0.45f, 0.25f, 0.10f);
    glBegin(GL_POLYGON);
    glVertex2f(centerX + dx - width / 2.0f + 0.06f, topY - depth);
    glVertex2f(centerX + dx - width / 2.0f + 0.06f + legW, topY - depth);
    glVertex2f(centerX + dx - width / 2.0f + 0.06f + legW, topY - depth - legH);
    glVertex2f(centerX + dx - width / 2.0f + 0.06f, topY - depth - legH);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(centerX + dx + width / 2.0f - 0.06f - legW, topY - depth);
    glVertex2f(centerX + dx + width / 2.0f - 0.06f, topY - depth);
    glVertex2f(centerX + dx + width / 2.0f - 0.06f, topY - depth - legH);
    glVertex2f(centerX + dx + width / 2.0f - 0.06f - legW, topY - depth - legH);
    glEnd();

    // starting point for plates
    float start = centerX + dx - width / 2.0f + 0.15f;

    for (int i = 0; i < 5; ++i) {

        if (i == 2) continue; // skip middle plate

        float px = start + i * ((width - 0.3f) / 4.0f);
        float py = topY - depth * 0.4f;

        drawPlateWithFood(px, py, 1.0f);
        drawGlass(px + 0.12f, py - 0.02f, 0.08f);
    }

    // centerpiece items (also shifted left)
    drawRoastChicken(centerX + dx - 0.09f, topY - depth * 0.09f, 0.11f);
    drawCake(centerX + dx + 0.13f, topY - depth * 0.2f, 0.15f);
}


// NEW: kitchen cabinets and refrigerator
void drawCabinetsAndFridge() {
    // refrigerator on the right (use global footprint so clicks match drawing)
    float rx1 = fridgeRX1, ry1 = fridgeRY1, rx2 = fridgeRX2, ry2 = fridgeRY2;

    // fridge body (slightly tinted metallic)
    glColor3f(0.92f, 0.95f, 0.97f);
    drawRect(rx1, ry2, rx2, ry1);

    // subtle side panel to suggest depth
    glColor3f(0.82f, 0.88f, 0.92f);
    drawRect(rx2 - 0.05f, ry2, rx2, ry1); // thin right-side highlight

    // fridge outline
    glColor3f(0.5f, 0.55f, 0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(rx1, ry2); glVertex2f(rx2, ry2);
    glVertex2f(rx2, ry1); glVertex2f(rx1, ry1);
    glEnd();

    // fridge handles (top and main)
    glColor3f(0.6f, 0.6f, 0.6f);
    drawRect(rx2 - 0.04f, ry2 - 0.10f, rx2 - 0.02f, ry2 - 0.02f);
    drawRect(rx2 - 0.04f, ry2 - 0.30f, rx2 - 0.02f, ry2 - 0.20f);

    // freezer divider line
    glColor3f(0.72f, 0.74f, 0.77f);
    glBegin(GL_LINES);
    glVertex2f(rx1 + 0.02f, ry2 - 0.20f); glVertex2f(rx2 - 0.02f, ry2 - 0.20f);
    glEnd();

    // small shadow at bottom of fridge
    glColor3f(0.2f, 0.18f, 0.16f);
    glBegin(GL_POLYGON);
    glVertex2f(rx1, ry1); glVertex2f(rx2, ry1);
    glVertex2f(rx2, ry1 - 0.02f); glVertex2f(rx1, ry1 - 0.02f);
    glEnd();

    // Cabinets placed directly to the LEFT (beside) of the refrigerator
    float cabGap = 0.02f;                  // small gap between fridge and cabinets
    float cabWidth = 0.40f;                // total cabinet block width
    float cabHeight = 0.16f;               // cabinet block height
    float cabRight = rx1 - cabGap;         // right edge of cabinets (touching fridge)
    float cabLeft = cabRight - cabWidth;   // left edge
    // align cabinet top slightly below fridge top for nicer composition
    float cabTop = ry2 - 0.04f;
    float cabBottom = cabTop - cabHeight;

    glColor3f(0.82f, 0.78f, 0.72f);
    // three cabinet doors across the block
    int doors = 3;
    float doorW = (cabWidth - 0.04f) / doors;
    float doorH = cabHeight - 0.02f;
    for (int i = 0; i < doors; ++i) {
        float cx = cabLeft + 0.02f + i * (doorW + 0.01f);
        float cyTop = cabTop;
        float cyBottom = cyTop - doorH;
        // door panel
        drawRect(cx, cyTop, cx + doorW, cyBottom);

        // door outline
        glColor3f(0.60f, 0.55f, 0.48f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cyTop); glVertex2f(cx + doorW, cyTop);
        glVertex2f(cx + doorW, cyBottom); glVertex2f(cx, cyBottom);
        glEnd();

        // handle (small horizontal bar centered vertically on door, toward right)
        float handleW = doorW * 0.22f;
        float handleH = doorH * 0.12f;
        float hx = cx + doorW - handleW - 0.04f;
        float hyTop = cyTop - doorH * 0.5f + handleH * 0.5f;
        glColor3f(0.58f, 0.58f, 0.6f);
        drawRect(hx, hyTop + handleH, hx + handleW, hyTop);

        // reset door color for next
        glColor3f(0.82f, 0.78f, 0.72f);
    }
}

// NEW: seven-segment digital digit drawing (simple rectangles)
void drawSegmentRect(float x, float y, float w, float h) {
    drawRect(x, y, x + w, y - h);
}

void drawDigit7(int d, float x, float y, float size, float thickness, const float color[3]) {
    // x,y = top-left of digit
    // layout parameters
    float segLen = size;
    float segTh = thickness;
    // segment positions:
    // a: top horizontal
    // b: top-right vertical
    // c: bottom-right vertical
    // d: bottom horizontal
    // e: bottom-left vertical
    // f: top-left vertical
    // g: middle horizontal
    bool segs[7];
    static const bool map[10][7] = {
        {1,1,1,1,1,1,0}, //0
        {0,1,1,0,0,0,0}, //1
        {1,1,0,1,1,0,1}, //2
        {1,1,1,1,0,0,1}, //3
        {0,1,1,0,0,1,1}, //4
        {1,0,1,1,0,1,1}, //5
        {1,0,1,1,1,1,1}, //6
        {1,1,1,0,0,0,0}, //7
        {1,1,1,1,1,1,1}, //8
        {1,1,1,1,0,1,1}  //9
    };
    for (int i = 0; i < 7; ++i) segs[i] = map[d][i];

    glColor3f(color[0], color[1], color[2]);

    // a
    if (segs[0]) drawSegmentRect(x + segTh, y, segLen, segTh);
    // b
    if (segs[1]) drawSegmentRect(x + segTh + segLen, y - segTh, segTh, segLen);
    // c
    if (segs[2]) drawSegmentRect(x + segTh + segLen, y - segTh - segLen - segTh, segTh, segLen);
    // d
    if (segs[3]) drawSegmentRect(x + segTh, y - 2 * (segTh + segLen), segLen, segTh);
    // e
    if (segs[4]) drawSegmentRect(x, y - segTh - segLen - segTh, segTh, segLen);
    // f
    if (segs[5]) drawSegmentRect(x, y - segTh, segTh, segLen);
    // g
    if (segs[6]) drawSegmentRect(x + segTh, y - segTh - segLen, segLen, segTh);
}

// Draw colon (two lit dots)
void drawColon(float x, float y, float size, const float color[3]) {
    glColor3f(color[0], color[1], color[2]);
    drawCircle(x, y - size * 0.7f, size * 0.08f, 12);
    drawCircle(x, y - size * 1.6f, size * 0.08f, 12);
}

// Draw digital clock reading "11:55 PM"
// Modified: draws black background and red digits; top-left now based on viewLeft/viewTop when called.
void drawDigitalClock(float topLeftX, float topLeftY, float size) {
    // size controls segment length; thickness set relative
    float thickness = size * 0.22f;
    const float colorOn[3] = { 1.0f, 0.12f, 0.12f }; // red digits
    const float colorOff[3] = { 0.2f, 0.2f, 0.1f };

    // layout metrics for background
    float spacing = size * 0.18f;
    float segLen = size;
    float segTh = thickness;
    // estimate total width: 4 digits + colon
    float totalW = 4.0f * (segLen + segTh) + 4.0f * spacing + segTh;
    float rectLeft = topLeftX - segTh * 0.6f;
    float rectTop = topLeftY + segTh * 0.6f;
    float rectRight = topLeftX + totalW + segTh * 0.6f;
    float rectBottom = topLeftY - (2.0f * (segTh + segLen)) - segTh * 0.6f;

    // store clock rect for mouse picking
    clockLeft = rectLeft; clockTop = rectTop; clockRight = rectRight; clockBottom = rectBottom;

    // draw black background rectangle behind digits
    glColor3f(0.03f, 0.03f, 0.03f); // near-black
    drawRect(rectLeft, rectTop, rectRight, rectBottom);

    // small padding outline for readability
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(rectLeft, rectTop); glVertex2f(rectRight, rectTop);
    glVertex2f(rectRight, rectBottom); glVertex2f(rectLeft, rectBottom);
    glEnd();

    // decide digits based on clockAtMidnight
    int d1, d2, d3, d4; // digits left-to-right
    const char* ampm = "PM";
    if (clockAtMidnight) {
        // 12:00 AM
        d1 = 1; d2 = 2; d3 = 0; d4 = 0;
        ampm = "AM";
    }
    else {
        // default 11:59 PM
        d1 = 1; d2 = 1; d3 = 5; d4 = 9;
        ampm = "PM";
    }

    // draw digits on top
    float x = topLeftX;
    float y = topLeftY;

    drawDigit7(d1, x, y, size, thickness, colorOn);
    x += size + thickness + spacing;
    drawDigit7(d2, x, y, size, thickness, colorOn);
    x += size + thickness + spacing * 0.8f;
    // colon
    drawColon(x + 0.02f, y - size * 0.2f, size, colorOn);
    x += spacing + thickness;
    drawDigit7(d3, x, y, size, thickness, colorOn);
    x += size + thickness + spacing;
    drawDigit7(d4, x, y, size, thickness, colorOn);

    // AM/PM label (small, light color on black)
    glColor3f(1.0f, 0.9f, 0.6f);
    drawText(rectRight - 0.08f, topLeftY - size * 0.9f, ampm);
}

// Scenes
void drawTitleScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    // NIGHT: deep sky base
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.02f, 0.03f, 0.12f); // near-black navy
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, viewTop);
    glVertex2f(viewLeft, viewTop);
    glEnd();

    // moon and stars
    drawMoon(-0.7f, 0.75f, 0.08f);

    // starfield (sparser)
    drawStar(-0.86f, 0.82f, 0.014f);
    drawStar(-0.78f, 0.76f, 0.010f);
    drawStar(-0.60f, 0.84f, 0.012f);
    drawStar(-0.45f, 0.78f, 0.009f);
    drawStar(-0.18f, 0.83f, 0.010f);
    drawStar(0.02f, 0.88f, 0.006f);
    drawStar(0.25f, 0.80f, 0.011f);
    drawStar(0.55f, 0.86f, 0.013f);

    // ground / street (darker)
    glColor3f(0.04f, 0.04f, 0.06f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.25f);
    glVertex2f(viewLeft, -0.25f);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.2f);
    float bx = 0.45f;
    float by = 0.45f;
    float br = 0.22f;

    drawThoughtBubble(bx, by, br);
    drawText(bx - 0.19f, by + 0.02f, "My new year's");
    drawText(bx - 0.18f, by - 0.06f, "resolution is...");

    drawKevin(0.0f, -0.2f, 0.9f);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(-0.5f, 0.4f, "Kevin's New Year");
    drawText(-0.5f, -0.4f, "Press ENTER to see Kevin's resolution...");

    glutSwapBuffers();
}

void drawThinkingScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    // wall (dining room warm tone) -> fill entire view
    glColor3f(0.98f, 0.95f, 0.92f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, viewTop);
    glVertex2f(viewLeft, viewTop);
    glEnd();

    // window -> night sky (darker)
    glColor3f(0.05f, 0.08f, 0.20f); // deep night blue
    glBegin(GL_POLYGON);
    glVertex2f(-0.9f, 0.2f); glVertex2f(-0.4f, 0.2f);
    glVertex2f(-0.4f, 0.8f); glVertex2f(-0.9f, 0.8f);
    glEnd();

    // stars and moon inside window (glowing)
    drawStar(-0.86f, 0.75f, 0.012f);
    drawStar(-0.82f, 0.70f, 0.008f);
    drawStar(-0.78f, 0.78f, 0.009f);
    drawStar(-0.73f, 0.69f, 0.006f);
    drawStar(-0.66f, 0.80f, 0.010f);
    drawStar(-0.61f, 0.72f, 0.007f);
    drawStar(-0.50f, 0.76f, 0.009f);
    drawMoon(-0.54f, 0.74f, 0.06f);

    // digital clock on upper-left. uses viewLeft/viewTop to anchor.
    drawDigitalClock(viewLeft + 0.06f, viewTop - 0.06f, 0.06f);

    // cabinets and fridge on right
    drawCabinetsAndFridge();

    // label by refrigerator to indicate clickability
    glColor3f(0.05f, 0.05f, 0.12f);
    // place label just to the left of fridge top
    drawText(0.9f, 0.02f, "Click on fridge to");
    drawText(0.9f, -0.05f, "have a drink instead");

    // floor / rug
    glColor3f(0.88f, 0.82f, 0.75f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.4f); // keep previous top-of-floor value to preserve layout
    glVertex2f(viewLeft, -0.4f);
    glEnd();

    // chandelier above (animated swing)
    drawChandelier(0.0f, 0.85f, 1.1f, chandelierPhase);

    // dining table with many foods
    float tableTopY = -0.05f; // slightly above Kevin's feet
    drawDiningTable(0.0f, tableTopY, 1.6f, 0.28f);

    // Kevin placed near the table (walking/standing) -- use animated X
    drawKevin(kevinPosX, kevinPosY, 0.98f);

    // draw drink (glass) if Kevin has it or is drinking
    if (kevinHasDrink || kevinAction == K_DRINKING) {
        // hand and mouth positions relative to Kevin
        float scale = 0.98f;
        float handX = kevinPosX + 0.12f * scale;
        float handY = kevinPosY + 0.24f * scale;
        float mouthX = kevinPosX + 0.0f * scale;
        float mouthY = kevinPosY + 0.20f * scale;

        float gx = handX, gy = handY;
        if (kevinAction == K_DRINKING) {
            // animate glass moving from hand to mouth over drinkTimer (0..80)
            float t = (float)drinkTimer / 60.0f; // 0..~1.3
            if (t > 1.0f) t = 1.0f;
            gx = handX + (mouthX - handX) * t;
            gy = handY + (mouthY - handY) * t;
            // tilt (smaller height value) to suggest sipping
            drawGlass(gx, gy, 0.045f * (1.0f - 0.15f * t));
        }
        else {
            // holding: draw glass at hand
            drawGlass(gx, gy, 0.05f);
        }
    }

    // show the thought bubble only after Kevin has finished the initial walk to center
    if (initialWalkDone && kevinAction == K_IDLE) {
        // compute bobbing offset for bubble
        float bob = sinf(bubblePhase) * 0.02f;
        float bubbleCx = 0.45f;
        float bubbleCy = 0.45f + bob;
        float bubbleR = 0.22f;

        // bubble outline and trail
        drawThoughtBubble(bubbleCx, bubbleCy, bubbleR);

        // inside bubble: mini-Kevin lifting weights (animated)
        drawMiniKevinLift(bubbleCx, bubbleCy - 0.03f, 0.9f, liftPhase);

        glColor3f(0, 0, 0);
        drawText(0.2f, 0.2f, "My resolution is to eat");
        drawText(0.4f, 0.1f, "healthy and be fit!");
        drawText(-1.0f, -0.7f, "December 31st: Kevin has come up with a resolution just before new year");
        drawText(-0.4f, -0.9f, "Press ENTER to continue...");
    }


    glutSwapBuffers();
}

// simple firework draw (radial burst + glow)
void drawFirework(float cx, float cy, float baseR, float phase, const float color[3]) {
    // glow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color[0], color[1], color[2], 0.12f);
    drawCircle(cx, cy, baseR * 2.4f, 24);
    glColor4f(color[0], color[1], color[2], 0.08f);
    drawCircle(cx, cy, baseR * 3.6f, 18);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // core burst (animated radius)
    float r = baseR * (0.6f + 0.8f * (0.5f + 0.5f * sinf(phase)));
    int spikes = 12;
    glColor3f(color[0], color[1], color[2]);
    glBegin(GL_LINES);
    for (int i = 0; i < spikes; ++i) {
        float a = 2.0f * 3.1415926f * i / spikes;
        float ex = cosf(a) * r;
        float ey = sinf(a) * r;
        glVertex2f(cx, cy);
        glVertex2f(cx + ex, cy + ey);
    }
    glEnd();

    // small spark dots
    for (int i = 0; i < 8; ++i) {
        float a = 2.0f * 3.1415926f * i / 8.0f + phase * 0.3f;
        float px = cx + cosf(a) * r * 1.15f;
        float py = cy + sinf(a) * r * 1.15f;
        glColor3f(fminf(1.0f, color[0] + 0.2f), fminf(1.0f, color[1] + 0.2f), fminf(1.0f, color[2] + 0.2f));
        drawCircle(px, py, baseR * 0.12f, 10);
    }
    glDisable(GL_BLEND);
}

// forward declaration: actual drawNextScene is defined later (detailed version)
void drawNextScene();

// --- forward declarations for neighborhood data used by updateAnimation/mouse handler ---
// (actual definitions appear later in the file)
extern const int HOUSE_COUNT;
extern float houseX[];
extern float houseY[];
extern float houseW[];
extern float houseH[];
extern bool friendActivated[];
extern bool friendWalkingArr[];
extern float friendPosX[];
extern float friendPosY[];
extern float friendTargetX[];
extern float friendTargetY[];   // <- add this
extern float friendSpeedArr[];

extern float nextKevinX;        // <- add this
extern float nextKevinY;        // <- add this

void drawReturnKitchenScene() {
    // BACKGROUND (same kitchen scene)
    glColor3f(0.98f, 0.95f, 0.92f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, viewTop);
    glVertex2f(viewLeft, viewTop);
    glEnd();

    // window -> night sky (darker)
    glColor3f(0.05f, 0.08f, 0.20f); // deep night blue
    glBegin(GL_POLYGON);
    glVertex2f(-0.9f, 0.2f); glVertex2f(-0.4f, 0.2f);
    glVertex2f(-0.4f, 0.8f); glVertex2f(-0.9f, 0.8f);
    glEnd();

    // stars and moon inside window (glowing)
    drawStar(-0.86f, 0.75f, 0.012f);
    drawStar(-0.82f, 0.70f, 0.008f);
    drawStar(-0.78f, 0.78f, 0.009f);
    drawStar(-0.73f, 0.69f, 0.006f);
    drawStar(-0.66f, 0.80f, 0.010f);
    drawStar(-0.61f, 0.72f, 0.007f);
    drawStar(-0.50f, 0.76f, 0.009f);
    drawMoon(-0.54f, 0.74f, 0.06f);

    drawCabinetsAndFridge();

    // floor / rug
    glColor3f(0.88f, 0.82f, 0.75f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.4f);
    glVertex2f(viewLeft, -0.4f);
    glEnd();

    // chandelier above (animated swing)
    drawChandelier(0.0f, 0.85f, 1.1f, chandelierPhase);

    float tableTopY = -0.05f;
    drawDiningTable(0.0f, tableTopY, 1.6f, 0.28f);
    //
    // KEVIN ANIMATED MOVEMENT
    //
    // Phase 0: Kevin moves left → center
    if (movePhase == 0) {
        if (kevinX < 0.0f) {
            kevinX += 0.01f;   // walking speed
        }
        else {
            movePhase = 1;     // reached center
        }
    }

    // Phase 2: Kevin moves center → right
    if (movePhase == 2) {
        if (kevinX > -1.2f) {   // move off-screen
            kevinX -= 0.01f;
        }
        else {
            // Done leaving → move to next scene
            showKitchenReturn = false;
            showPlateScene = true;
        }
    }

    // Draw Kevin at animated position
    drawKevin(kevinX, -0.38f, 1.0f);


    //
    // SPEECH BUBBLE (only when Kevin is centered)
    //
    if (movePhase == 1) {
        float bx = 0.45f;
        float by = 0.45f;
        float br = 0.22f;

        drawThoughtBubble(bx, by, br);
        drawText(bx - 0.03f, by + 0.02f, "...");


        glColor3f(0, 0, 0);
        drawText(-0.7f, -0.9f, "Press ENTER to find out Kevin's choice...");
    }

    //
    // INSTRUCTIONS
    //
    if (movePhase == 2) {
        float bx = 0.45f;
        float by = 0.45f;
        float br = 0.22f;

        drawThoughtBubble(bx, by, br);
        drawText(bx - 0.03f, by + 0.02f, "no!");
    }

    glColor3f(0, 0, 0);
    drawText(-0.9f, -0.7f, "January 1st: Kevin saw the delicious food and was tempted...");


    glutSwapBuffers();
}



void drawPlateScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    // NIGHT: deep sky base
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.02f, 0.03f, 0.12f); // near-black navy
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, viewTop);
    glVertex2f(viewLeft, viewTop);
    glEnd();

    // subtle horizon glow for city silhouette
    glColor3f(0.06f, 0.06f, 0.12f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.6f);
    glVertex2f(viewLeft, -0.6f);
    glEnd();

    // moon and stars
    drawMoon(-0.7f, 0.75f, 0.08f);
    // starfield (sparser)
    drawStar(-0.86f, 0.82f, 0.014f);
    drawStar(-0.78f, 0.76f, 0.010f);
    drawStar(-0.60f, 0.84f, 0.012f);
    drawStar(-0.45f, 0.78f, 0.009f);
    drawStar(-0.18f, 0.83f, 0.010f);
    drawStar(0.02f, 0.88f, 0.006f);
    drawStar(0.25f, 0.80f, 0.011f);
    drawStar(0.55f, 0.86f, 0.013f);

    if (fireworksActive) {
        const float fwColors[4][3] = {
            {1.0f, 0.4f, 0.2f},
            {0.9f, 0.9f, 0.25f},
            {0.4f, 0.9f, 0.95f},
            {0.9f, 0.5f, 0.95f}
        };
        float fwX[5] = { -0.4f, -0.05f, 0.2f, 0.5f, 0.85f };
        float fwY[5] = { 0.6f, 0.78f, 0.75f, 0.7f, 0.72f };
        float fwR[5] = { 0.05f, 0.07f, 0.06f, 0.055f, 0.065f };
        for (int i = 0; i < 5; ++i) {
            int ci = i % 4;
            drawFirework(fwX[i], fwY[i], fwR[i], fireworksPhase + i * 1.3f, fwColors[ci]);
        }
    }

    // houses -- draw darker silhouettes with lit windows
    for (int i = 0; i < HOUSE_COUNT; ++i) {

        float hx = houseX[i], hy = houseY[i], hw = houseW[i], hh = houseH[i];

        // generate a subtle dark color based on i
        float r = 0.12f + 0.03f * (i % 5);
        float g = 0.10f + 0.025f * ((i + 2) % 5);
        float b = 0.14f + 0.035f * ((i + 4) % 5);

        glColor3f(r, g, b);
        drawRect(hx, hy, hx + hw, hy + hh);

        // windows stay the same
        glColor3f(1.0f, 0.84f, 0.35f);
        drawRect(hx + hw * 0.12f, hy + hh * 0.6f, hx + hw * 0.25f, hy + hh * 0.45f);
        drawRect(hx + hw * 0.62f, hy + hh * 0.5f, hx + hw * 0.75f, hy + hh * 0.35f);
    }

    // ground / street (darker)
    glColor3f(0.04f, 0.04f, 0.06f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.75f);
    glVertex2f(viewLeft, -0.75f);
    glEnd();

    // Kevin centered
    glColor3f(0.2f, 0.3f, 0.8f);
    drawCircle(0.0f, -0.6f, 0.15f, 30);

    drawKevin(0.0f, -0.7f, 1.0f);

    // plate in hand
    drawPlateWithFood(0.12f, -0.65f);

    // speech bubble: "I'll do it next year"
    float bx = 0.45f;
    float by = -0.1f;
    float br = 0.22f;

    drawThoughtBubble(bx, by, br);
    drawText(bx - 0.18f, by + 0.02f, "I'll do it");
    drawText(bx - 0.18f, by - 0.04f, "next year...");

    glColor3f(1, 1, 1);
    drawText(-0.2f, 0.0f, "Oh Kevin...");

    glutSwapBuffers();
}



// animation timer
void updateAnimation(int value) {
    if (true) { // ensure timer keeps running while scene is active
        // update visual phases even if bubble isn't shown yet
        bubblePhase += 0.06f;        // bob speed
        liftPhase += 0.25f;         // lift speed
        chandelierPhase += 0.02f;   // chandelier swing
        fireworksPhase += 0.12f;    // NEW: animate fireworks

        // advance Kevin if walking for both initial and interactive sequences
        if (kevinWalking) {
            float dx = kevinTargetX - kevinPosX;
            float step = kevinSpeed;
            if (fabsf(dx) <= step) {
                kevinPosX = kevinTargetX;
                kevinWalking = false;

                // If this was the initial entrance walk, enable the bubble now
                if (initialWalkStarted && !initialWalkDone && fabsf(kevinPosX - 0.0f) < 0.001f) {
                    initialWalkDone = true;
                    initialWalkStarted = false;
                    animateBubble = true;      // show the thought bubble after Kevin stops in middle
                }
            }
            else {
                kevinPosX += (dx > 0.0f ? step : -step);
            }
        }

        // advance friends walking (now X+Y so they come from the house side toward Kevin)
        for (int i = 0; i < HOUSE_COUNT; ++i) {
            if (friendWalkingArr[i]) {
                float dx = friendTargetX[i] - friendPosX[i];
                float dy = friendTargetY[i] - friendPosY[i];
                float dist = sqrtf(dx * dx + dy * dy);
                float step = friendSpeedArr[i];
                if (dist <= step || dist == 0.0f) {
                    friendPosX[i] = friendTargetX[i];
                    friendPosY[i] = friendTargetY[i];
                    friendWalkingArr[i] = false; // friend now stands beside Kevin
                }
                else {
                    float nx = dx / dist;
                    float ny = dy / dist;
                    friendPosX[i] += nx * step;
                    friendPosY[i] += ny * step;
                }
            }
        }

        // handle interactive action state machine
        if (kevinAction == K_WALK_TO_FRIDGE && !kevinWalking) {
            // arrived near fridge -> pick drink
            kevinAction = K_PICK_DRINK;
            pickTimer = 0;
        }
        else if (kevinAction == K_PICK_DRINK) {
            pickTimer++;
            if (pickTimer == 1) {
                // simulate grabbing immediately: Kevin now "has" the drink
                kevinHasDrink = true;
            }
            // after short delay, walk back to center
            if (pickTimer > 20) {
                kevinAction = K_WALK_BACK;
                kevinTargetX = 0.0f;
                kevinSpeed = fabs(kevinTargetX - kevinPosX) / 60.0f;
                kevinWalking = true;
            }
        }
        else if (kevinAction == K_WALK_BACK && !kevinWalking) {
            // arrived center with drink -> start drinking
            kevinAction = K_DRINKING;
            drinkTimer = 0;
        }
        else if (kevinAction == K_DRINKING) {
            drinkTimer++;
            // during drinking animate for ~80 frames then finish
            if (drinkTimer > 80) {
                kevinAction = K_IDLE;
                kevinHasDrink = false; // finished
            }
        }

        // keep phases bounded
        if (bubblePhase > 10000.0f) bubblePhase = 0.0f;
        if (liftPhase > 10000.0f) liftPhase = 0.0f;
        if (chandelierPhase > 10000.0f) chandelierPhase = 0.0f;
        if (fireworksPhase > 10000.0f) fireworksPhase = 0.0f;

        glutPostRedisplay();
        glutTimerFunc(30, updateAnimation, 0);
    }
}

// input
void keyPressed(unsigned char key, int x, int y) {
    if (key == 13) { // ENTER
        if (showTitle) {
            showTitle = false;
            showThinking = true;
            // delay bubble until Kevin finishes walking to center
            animateBubble = false;

            // initialize Kevin walking from left off-screen toward the middle
            kevinPosX = viewLeft - 0.12f; // start a bit off the left edge of the current view
            kevinPosY = -0.38f;
            kevinTargetX = 0.0f;          // center of the scene
            kevinSpeed = fabs(kevinTargetX - kevinPosX) / 90.0f; // reach in ~90 frames (~2.7s)
            kevinWalking = true;

            // mark that the initial walk was started
            initialWalkStarted = true;
            initialWalkDone = false;

            glutTimerFunc(30, updateAnimation, 0);
        }
        else if (showThinking) {
            showThinking = false;
            showNext = true;
            animateBubble = false;
            kevinWalking = false;
        }
        else if (showNext && key == 13) {
            showNext = false;
            showKitchenReturn = true;
        }
        else if (showKitchenReturn && key == 13) {
            if (movePhase == 1) {
                // Start moving from center → right
                movePhase = 2;
            }
        }

    }

    glutPostRedisplay();
}


// map window (mouse) coords to world coords using current view extents
void windowToWorld(int mx, int my, float& wx, float& wy) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    float nx = (float)mx / (float)w;           // 0..1 left->right
    float ny = 1.0f - (float)my / (float)h;    // 0..1 bottom->top
    wx = viewLeft + nx * (viewRight - viewLeft);
    wy = viewBottom + ny * (viewTop - viewBottom);
}

// ...existing code...
void mouseClick(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

    float wx, wy;
    windowToWorld(x, y, wx, wy);

    // if user clicked the clock area, toggle to midnight and trigger fireworks
    if (wx >= clockLeft && wx <= clockRight && wy <= clockTop && wy >= clockBottom) {
        clockAtMidnight = true;
        fireworksActive = true;
        // ensure animation timer is running so fireworks animate
        glutTimerFunc(30, updateAnimation, 0);
        glutPostRedisplay();
        return;
    }

    // only allow house->friend interaction in the outside scene
    if (showNext) {
        for (int i = 0; i < HOUSE_COUNT; ++i) {
            float hx = houseX[i], hy = houseY[i], hw = houseW[i], hh = houseH[i];
            if (wx >= hx && wx <= hx + hw && wy >= hy && wy <= hy + hh) {
                if (!friendActivated[i] && !friendWalkingArr[i]) {
                    // activate friend and make them walk toward Kevin's meeting point
                    friendActivated[i] = true;
                    friendWalkingArr[i] = true;

                    // start inside at the door center
                    float doorCenterX = hx + hw * 0.5f;
                    friendPosX[i] = doorCenterX;
                    friendPosY[i] = hy + 0.02f;

                    // choose horizontal slot beside Kevin based on how many friends already active
                    int slot = 0;
                    for (int j = 0; j < HOUSE_COUNT; ++j) if (friendActivated[j] && j != i) ++slot;

                    // wider spacing so friends stand beside Kevin (not overlapping)
                    float slotOffset = -0.33f + 0.22f * slot; // positions: -0.33, -0.11, 0.11, 0.33

                    friendTargetX[i] = nextKevinX + slotOffset;
                    friendTargetY[i] = nextKevinY; // align feet with Kevin

                    // set speed proportional to straight-line distance (arrive in ~60 frames)
                    float dx = friendTargetX[i] - friendPosX[i];
                    float dy = friendTargetY[i] - friendPosY[i];
                    friendSpeedArr[i] = sqrtf(dx * dx + dy * dy) / 60.0f;
                }
                return; // consumed click on a house
            }
        }
    }

    // then check fridge only when Kevin is idle at center (existing behavior)
    if (kevinWalking || kevinAction != K_IDLE) return;

    // check if click inside fridge rectangle (use globals)
    if (wx >= fridgeRX1 && wx <= fridgeRX2 && wy >= fridgeRY1 && wy <= fridgeRY2) {
        // start interactive sequence: Kevin walks to fridge, picks drink, returns and drinks
        kevinAction = K_WALK_TO_FRIDGE;
        // approach point a bit left of fridge so Kevin stands in front
        kevinTargetX = fridgeRX1 - 0.12f;
        // set movement speed (reach in ~60 frames)
        kevinSpeed = fabs(kevinTargetX - kevinPosX) / 60.0f;
        kevinWalking = true;
    }
}

// forward declaration to fix 'display' not declared error
void display();
// forward declare mouse handler so main can register it
void mouseClick(int button, int state, int x, int y);

// forward declare init() and reshape() used in main
void init();
void reshape(int w, int h);

int main(int argc, char** argv) {
    // pass both argc and argv to glutInit
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Kevin's New Year - Resolutions");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyPressed);
    glutMouseFunc(mouseClick); // register mouse click handler

    // timer started when ENTER is pressed to show thinking scene
    glutMainLoop();
    return 0;
}

void display() {
    if (showTitle) drawTitleScene();
    else if (showThinking) drawThinkingScene();
    else if (showNext) drawNextScene();
    else if (showKitchenReturn) drawReturnKitchenScene();
    else if (showPlateScene) drawPlateScene();
}

// Initialize GL state
void init() {
    // background default (title/scene will overwrite as needed)
    glClearColor(0.95f, 0.95f, 0.98f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ensure a sensible initial projection using the current view extents
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewLeft, viewRight, viewBottom, viewTop, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Handle window resize and update world extents so everything scales
void reshape(int w, int h) {
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    glViewport(0, 0, w, h);

    // keep vertical extents fixed and adjust horizontal extents by aspect ratio
    float aspect = (float)w / (float)h;
    viewTop = 1.0f;
    viewBottom = -1.0f;
    viewLeft = -aspect;
    viewRight = aspect;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewLeft, viewRight, viewBottom, viewTop, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// --- neighborhood + friend interaction state ---
const int HOUSE_COUNT = 4;
float houseX[HOUSE_COUNT] = { -0.9f, -0.5f, -0.1f, 0.35f };
float houseY[HOUSE_COUNT] = { -0.6f, -0.6f, -0.6f, -0.6f };
float houseW[HOUSE_COUNT] = { 0.5f, 0.42f, 0.42f, 0.42f };
float houseH[HOUSE_COUNT] = { 0.6f, 0.56f, 0.56f, 0.56f };

bool friendActivated[HOUSE_COUNT] = { false, false, false, false }; // friend has come out / is visible
bool friendWalkingArr[HOUSE_COUNT] = { false, false, false, false }; // friend currently walking out
float friendPosX[HOUSE_COUNT] = { 0 }, friendPosY[HOUSE_COUNT] = { 0 };
float friendTargetX[HOUSE_COUNT] = { 0 }, friendTargetY[HOUSE_COUNT] = { 0 }, friendSpeedArr[HOUSE_COUNT] = { 0 };

// meeting point for friends to walk to (Kevin's outside position)
float nextKevinX = 0.15f;
float nextKevinY = -0.75f;

// small friend drawing (simple style, smaller than Kevin)
void drawFriend(float x, float y, float scale = 1.0f) {
    // hair
    glColor3f(0.90f, 0.72f, 0.42f);
    drawCircle(x, y + 0.28f * scale, 0.13f * scale);
    // head
    glColor3f(1.0f, 0.85f, 0.65f);
    drawCircle(x, y + 0.25f * scale, 0.12f * scale);

    // eyes (matching Kevin style)
    float ex = 0.03f * scale, ey = 0.03f * scale;
    float er = 0.03f * scale, pr = 0.005f * scale;
    glColor3f(1, 1, 1); drawCircle(x - ex, y + 0.25f * scale + ey, er);
    glColor3f(0, 0, 0); drawCircle(x - ex, y + 0.25f * scale + ey, pr);
    glColor3f(1, 1, 1); drawCircle(x + ex, y + 0.25f * scale + ey, er);
    glColor3f(0, 0, 0); drawCircle(x + ex, y + 0.25f * scale + ey, pr);

    // mouth
    glColor3f(0.6f, 0.0f, 0.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x - 0.03f * scale, y + 0.22f * scale);
    glVertex2f(x, y + 0.20f * scale);
    glVertex2f(x + 0.03f * scale, y + 0.22f * scale);
    glEnd();

    // torso (different color so friends stand out)
    glColor3f(0.18f, 0.55f, 0.30f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.10f * scale, y);
    glVertex2f(x + 0.10f * scale, y);
    glVertex2f(x + 0.10f * scale, y + 0.20f * scale);
    glVertex2f(x - 0.10f * scale, y + 0.20f * scale);
    glEnd();

    // arms (both down)
    glColor3f(1.0f, 0.85f, 0.65f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.12f * scale, y + 0.18f * scale);
    glVertex2f(x - 0.10f * scale, y + 0.18f * scale);
    glVertex2f(x - 0.10f * scale, y + 0.05f * scale);
    glVertex2f(x - 0.12f * scale, y + 0.05f * scale);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x + 0.10f * scale, y + 0.18f * scale);
    glVertex2f(x + 0.12f * scale, y + 0.18f * scale);
    glVertex2f(x + 0.12f * scale, y + 0.05f * scale);
    glVertex2f(x + 0.10f * scale, y + 0.05f * scale);
    glEnd();

    // legs
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.08f * scale, y - 0.10f * scale);
    glVertex2f(x - 0.02f * scale, y - 0.10f * scale);
    glVertex2f(x - 0.02f * scale, y);
    glVertex2f(x - 0.08f * scale, y);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x + 0.02f * scale, y - 0.10f * scale);
    glVertex2f(x + 0.08f * scale, y - 0.10f * scale);
    glVertex2f(x + 0.08f * scale, y);
    glVertex2f(x + 0.02f * scale, y);
    glEnd();
}

// Minimal outside / next-scene renderer so drawNextScene is defined.
// Keeps visuals simple: sky, houses, Kevin meeting point and friends.
// ...existing code...
void drawNextScene() {
    // NIGHT: deep sky base
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.02f, 0.03f, 0.12f); // near-black navy
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, viewTop);
    glVertex2f(viewLeft, viewTop);
    glEnd();

    // draw the clock in the upper-left of the view too (keeps consistent)
    drawDigitalClock(viewLeft + 0.06f, viewTop - 0.06f, 0.06f);

    // subtle horizon glow for city silhouette
    glColor3f(0.06f, 0.06f, 0.12f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.6f);
    glVertex2f(viewLeft, -0.6f);
    glEnd();

    // moon and stars
    drawMoon(-0.7f, 0.75f, 0.08f);
    // starfield (sparser)
    drawStar(-0.86f, 0.82f, 0.014f);
    drawStar(-0.78f, 0.76f, 0.010f);
    drawStar(-0.60f, 0.84f, 0.012f);
    drawStar(-0.45f, 0.78f, 0.009f);
    drawStar(-0.18f, 0.83f, 0.010f);
    drawStar(0.02f, 0.88f, 0.006f);
    drawStar(0.25f, 0.80f, 0.011f);
    drawStar(0.55f, 0.86f, 0.013f);

    // fireworks: only draw when triggered by clicking the clock
    if (fireworksActive) {
        const float fwColors[4][3] = {
            {1.0f, 0.4f, 0.2f},
            {0.9f, 0.9f, 0.25f},
            {0.4f, 0.9f, 0.95f},
            {0.9f, 0.5f, 0.95f}
        };
        float fwX[5] = { -0.4f, -0.05f, 0.2f, 0.5f, 0.85f };
        float fwY[5] = { 0.6f, 0.78f, 0.75f, 0.7f, 0.72f };
        float fwR[5] = { 0.05f, 0.07f, 0.06f, 0.055f, 0.065f };
        for (int i = 0; i < 5; ++i) {
            int ci = i % 4;
            drawFirework(fwX[i], fwY[i], fwR[i], fireworksPhase + i * 1.3f, fwColors[ci]);
        }

        // show Happy New Year text big and centered-top
        glColor3f(1.0f, 0.95f, 0.6f);
        drawText(-0.30f, 0.45f, "HAPPY NEW YEAR!");
    }

    // houses -- draw darker silhouettes with lit windows
    for (int i = 0; i < HOUSE_COUNT; ++i) {

        float hx = houseX[i], hy = houseY[i], hw = houseW[i], hh = houseH[i];

        // generate a subtle dark color based on i
        float r = 0.12f + 0.03f * (i % 5);
        float g = 0.10f + 0.025f * ((i + 2) % 5);
        float b = 0.14f + 0.035f * ((i + 4) % 5);

        glColor3f(r, g, b);
        drawRect(hx, hy, hx + hw, hy + hh);

        // windows stay the same
        glColor3f(1.0f, 0.84f, 0.35f);
        drawRect(hx + hw * 0.12f, hy + hh * 0.6f, hx + hw * 0.25f, hy + hh * 0.45f);
        drawRect(hx + hw * 0.62f, hy + hh * 0.5f, hx + hw * 0.75f, hy + hh * 0.35f);
    }


    // ground / street (darker)
    glColor3f(0.04f, 0.04f, 0.06f);
    glBegin(GL_POLYGON);
    glVertex2f(viewLeft, viewBottom);
    glVertex2f(viewRight, viewBottom);
    glVertex2f(viewRight, -0.75f);

    glVertex2f(viewLeft, -0.75f);
    glEnd();

    // friends gathered and Kevin at meeting point (use existing positions)
    for (int i = 0; i < HOUSE_COUNT; ++i) {
        if (friendActivated[i] || friendWalkingArr[i]) {
            // give friends a small highlight to show celebration (outline)
            glColor3f(0.95f, 0.95f, 1.0f);
            // draw friend at full Kevin size with face
            drawFriend(friendPosX[i], friendPosY[i], 1.0f);
        }
    }
    // Kevin at meeting point
    drawKevin(nextKevinX, nextKevinY, 0.95f);

    // small instruction
    glColor3f(0.8f, 0.8f, 0.9f);
    drawText(-0.3f, 0.0f, "Click houses to invite friends to join the celebration");
    drawText(-1.0f, -0.9f, "Click clock on top left to celebrate the new year!                               Enter to continue...");

    glutSwapBuffers();
}
