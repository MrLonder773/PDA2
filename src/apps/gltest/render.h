#pragma once

#include <iostream>
#include <tgx.h>
#include <PDA2.h>

class load{
public:
    void init();
};

//Дальше идут функции для рендера

class render{
public:
    //void changeColor(float r, float g, float b);
    // im, screen_x/y, w/h приходят снаружи — render ничего не хранит сам
    void vertic(tgx::Image<tgx::RGB565>& im, int screen_x, int screen_y, int w, int h /*int x, int y*/);
};

//Не работает массив
//UPD: замена на glVertexPointer(2, GL_FLOAT, 0, &vertices);
//    GLfloat quadss[] =
//    {
//       glColor3f(1.0, 0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
//       glColor3f(0.0, 1.0f, 0.0f); glVertex2f(0.5f, 0.0f);
//       glColor3f(0.0, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
//       glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
//       glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(0.5f, 0.0f);
//       glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(0.5f, 0.5f);
//    };

/*
 * render.h
 * Здесь находится реализация базового рендера
 */
