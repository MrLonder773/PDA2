#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Path.h
//  Нормализация путей Fs. Общая для ESP32 и sim, чтобы
//  поведение не разъезжалось.
// ════════════════════════════════════════════════════════

#include <string>

// true  — путь допустим, out = ведущий '/', без '//', без хвостового '/' (кроме корня).
// false — путь null, пустой или содержит сегмент "." / ".."; out пуст.
inline bool fs_normPath(const char* path, std::string& out) {
    out.clear();
    if (!path || !*path) return false;
    const std::string in = path;
    out = "/";
    size_t i = 0;
    while (i < in.size()) {
        while (i < in.size() && in[i] == '/') ++i;
        size_t j = i;
        while (j < in.size() && in[j] != '/') ++j;
        if (j == i) break;
        const std::string seg = in.substr(i, j - i);
        if (seg == "." || seg == "..") { out.clear(); return false; }
        if (out.size() > 1) out.push_back('/');
        out += seg;
        i = j;
    }
    return true;
}