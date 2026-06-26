#include <math.h>
#include <stdio.h>
#include "gui.h"

static Vector2 vectorLerp(Vector2 a, Vector2 b, float t) {
    Vector2 result;
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    return result;
}

void initNodePositions(Vector2 positions[], int N) {
    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f + 20;
    float radius = 230.0f;

    for (int i = 0; i < N; i++) {
        float angle = (2.0f * PI * i) / N - PI / 2.0f;
        positions[i].x = centerX + cosf(angle) * radius;
        positions[i].y = centerY + sinf(angle) * radius;
    }
}

void initTravelerColors(Traveler travelers[], int travelerCount) {
    Color colors[MAX_TRAVELERS] = {
        RED, BLUE, GREEN, ORANGE, PURPLE,
        MAROON, DARKGREEN, DARKBLUE, PINK, GOLD
    };

    for (int i = 0; i < travelerCount; i++) {
        travelers[i].color = colors[i % MAX_TRAVELERS];
    }
}

void startMoveAnimation(Traveler *t, Vector2 from, Vector2 to, int weight) {
    t->moving = 1;
    t->entering = 0;
    t->waiting = 0;

    t->moveStartPosition = from;
    t->moveTargetPosition = to;
    t->moveStartTime = GetTime();

    if (weight <= 0) {
        weight = 1;
    }

    t->moveDuration = (double)(weight * EDGE_TIME_SCALE) / 1000.0;
}

void startEnterAnimation(Traveler *t, Vector2 from, Vector2 to) {
    t->entering = 1;
    t->moving = 0;
    t->waiting = 0;

    t->enterStartPosition = from;
    t->enterTargetPosition = to;
    t->enterStartTime = GetTime();
    t->enterDuration = (double)ENTER_ANIMATION_MS / 1000.0;
}

void updateTravelerAnimations(Traveler travelers[], int travelerCount) {
    double now = GetTime();

    for (int i = 0; i < travelerCount; i++) {
        Traveler *t = &travelers[i];

        if (t->moving) {
            double elapsed = now - t->moveStartTime;
            float progress = (float)(elapsed / t->moveDuration);

            if (progress >= 1.0f) {
                progress = 1.0f;
                t->moving = 0;
            }

            t->position = vectorLerp(t->moveStartPosition,
                                     t->moveTargetPosition,
                                     progress);
        }

        if (t->entering) {
            double elapsed = now - t->enterStartTime;
            float progress = (float)(elapsed / t->enterDuration);

            if (progress >= 1.0f) {
                progress = 1.0f;
                t->entering = 0;
            }

            t->position = vectorLerp(t->enterStartPosition,
                                     t->enterTargetPosition,
                                     progress);
        }
    }
}

static void drawArrow(Vector2 start, Vector2 end, Color color) {
    Vector2 direction = {
        end.x - start.x,
        end.y - start.y
    };

    float length = sqrtf(direction.x * direction.x +
                         direction.y * direction.y);

    if (length == 0) {
        return;
    }

    direction.x /= length;
    direction.y /= length;

    Vector2 newStart = {
        start.x + direction.x * 32,
        start.y + direction.y * 32
    };

    Vector2 newEnd = {
        end.x - direction.x * 32,
        end.y - direction.y * 32
    };

    DrawLineEx(newStart, newEnd, 3, color);

    Vector2 left = {
        newEnd.x - direction.x * 16 - direction.y * 8,
        newEnd.y - direction.y * 16 + direction.x * 8
    };

    Vector2 right = {
        newEnd.x - direction.x * 16 + direction.y * 8,
        newEnd.y - direction.y * 16 - direction.x * 8
    };

    DrawTriangle(newEnd, left, right, color);
}

static void drawGraph(GraphData *data, Vector2 positions[]) {
    for (int i = 0; i < data->M; i++) {
        Edge e = data->edges[i];

        Vector2 start = positions[e.src];
        Vector2 end = positions[e.dst];

        drawArrow(start, end, DARKGRAY);

        Vector2 mid = {
            (start.x + end.x) / 2.0f,
            (start.y + end.y) / 2.0f
        };

        char weightText[20];
        sprintf(weightText, "%d", e.weight);

        DrawCircleV(mid, 14, RAYWHITE);
        DrawText(weightText, (int)mid.x - 5, (int)mid.y - 8, 16, BLACK);
    }

    for (int i = 0; i < data->N; i++) {
        DrawCircleV(positions[i], 30, SKYBLUE);
        DrawCircleLines((int)positions[i].x,
                        (int)positions[i].y,
                        30,
                        DARKBLUE);

        char nodeText[20];
        sprintf(nodeText, "%d", i);

        DrawText(nodeText,
                 (int)positions[i].x - 6,
                 (int)positions[i].y - 10,
                 22,
                 BLACK);
    }
}

static void drawTravelers(GraphData *data) {
    for (int i = 0; i < data->travelerCount; i++) {
        Traveler *t = &data->travelers[i];

        if (t->noPath) {
            continue;
        }

        DrawCircleV(t->position, 12, t->color);
        DrawCircleLines((int)t->position.x,
                        (int)t->position.y,
                        12,
                        BLACK);

        char label[40];

if (t->pid > 0) {
    sprintf(label, "T%d PID:%d", i + 1, t->pid);
} else {
    sprintf(label, "T%d", i + 1);
}

DrawText(label,
         (int)t->position.x - 35,
         (int)t->position.y - 32,
         14,
         BLACK);

        if (t->waiting) {
            DrawText("WAIT",
                     (int)t->position.x - 20,
                     (int)t->position.y + 18,
                     16,
                     RED);
        }

        if (t->finished) {
            DrawText("DONE",
                     (int)t->position.x - 22,
                     (int)t->position.y + 18,
                     16,
                     DARKGREEN);
        }
    }
}

static void drawInfoPanel(GraphData *data,
                          SchedulerType scheduler,
                          NodeQueue queues[]) {
    DrawRectangle(10, 10, 280, 160, Fade(LIGHTGRAY, 0.85f));
    DrawRectangleLines(10, 10, 280, 160, GRAY);

    char title[80];
    sprintf(title, "Scheduler: %s", schedulerName(scheduler));
    DrawText(title, 25, 25, 22, DARKBLUE);

    int y = 60;

    for (int i = 0; i < data->N && y < 150; i++) {
        if (queues[i].count > 0 || queues[i].occupied) {
            char line[100];

            sprintf(line,
                    "Node %d | occupied=%d | waiting=%d",
                    i,
                    queues[i].occupied,
                    queues[i].count);

            DrawText(line, 25, y, 16, BLACK);
            y += 20;
        }
    }
}

void drawSimulation(GraphData *data,
                    Vector2 nodePositions[],
                    SchedulerType scheduler,
                    NodeQueue queues[]) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawText("Operating Systems Project - Milestone 7",
             300, 20, 24, DARKBLUE);

    drawGraph(data, nodePositions);
    drawTravelers(data);
    drawInfoPanel(data, scheduler, queues);

    EndDrawing();
}