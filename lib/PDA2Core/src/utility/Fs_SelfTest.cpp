// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_SelfTest.cpp
//  Самотест Fs на реальном железе (LittleFS и SD).
//  ВЫКЛЮЧЕН по умолчанию. Включение: добавить -D PDA2_FS_SELFTEST
//  в build_flags (platformio.ini); после проверки убрать.
//  Запускается из Fs_Class::begin(), результат — в лог (тег "fstest").
//
//  Безопасность: работает ТОЛЬКО в папке /_fstest каждого хранилища
//  (перед стартом удаляет остатки этой папки, в конце удаляет её).
//  Корень и чужие файлы не трогает.
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"

#ifdef PDA2_FS_SELFTEST

#include "../pda2_config.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace {

int         g_pass  = 0;
int         g_fail  = 0;
const char* g_label = "";

#define CHK(c) do { if (c) ++g_pass; else { ++g_fail; \
    PDA_LOGE("fstest", "FAIL [%s] строка %d: %s", g_label, __LINE__, #c); } } while (0)

// путь внутри тестовой папки; временная строка живёт до конца выражения
#define PP(x) (std::string(root) + (x)).c_str()

bool has(const std::vector<String>& v, const char* p) {
    for (const auto& x : v)
        if (x == p) return true;
    return false;
}

void run(FsSource& s, const char* label) {
    g_label = label;
    if (!s.ok()) {
        PDA_LOGI("fstest", "[%s] недоступно, пропуск", label);
        return;
    }
    const std::string rootStr = "/_fstest";
    const char* root = rootStr.c_str();
    const int pass0 = g_pass, fail0 = g_fail;
    PDA_LOGI("fstest", "[%s] старт. Свободно: %u МБ", label,
             (unsigned)(s.freeSpace() / (1024ULL * 1024ULL)));

    s.removeAll(root);                                  // остатки прошлого запуска (только наша папка)

    // mkdir / write / append / read / size
    CHK(s.mkdir(PP("/a/b/c")));
    CHK(s.isDir(PP("/a/b/c")));
    CHK(s.mkdir(PP("/a/b/c")));                         // повтор — успех
    CHK(s.write(PP("/a/b/c/x.txt"), "hello"));
    CHK(!s.mkdir(PP("/a/b/c/x.txt")));                  // на месте файл
    CHK(!s.write(PP("/nodir/x.txt"), "z"));             // родителя нет
    CHK(s.append(PP("/a/b/c/x.txt"), " world"));
    CHK(s.read(PP("/a/b/c/x.txt")) == "hello world");
    CHK(s.size(PP("/a/b/c/x.txt")) == 11);
    CHK(s.append(PP("/a/new.txt"), "n"));               // дозапись создаёт файл
    CHK(s.read(PP("/a/new.txt")) == "n");
    CHK(s.write(PP("/a/empty.txt"), ""));               // пустая запись — успех
    CHK(s.exists(PP("/a/empty.txt")));
    CHK(s.size(PP("/a/empty.txt")) == 0);

    // copy
    CHK(s.copy(PP("/a/b/c/x.txt"), PP("/a/copy.txt")));
    CHK(s.read(PP("/a/copy.txt")) == "hello world");
    CHK(!s.copy(PP("/a/copy.txt"), PP("/a/copy.txt")));   // сам в себя — отказ
    CHK(s.read(PP("/a/copy.txt")) == "hello world");      // источник цел
    CHK(!s.copy(PP("/a/b"), PP("/a/bcopy")));              // папку копировать нельзя
    CHK(!s.copy(PP("/a/missing.txt"), PP("/a/m2.txt")));
    CHK(!s.exists(PP("/a/m2.txt")));

    // find / ls / dirSize
    auto r = s.find(root, "X.TXT");
    CHK(r.size() == 1 && has(r, PP("/a/b/c/x.txt")));     // регистр латиницы не важен
    CHK(has(s.find(root, "b"), PP("/a/b")));              // папки находятся тоже
    CHK(s.find(root, "").empty());
    CHK(s.find(PP("/nodir"), "x").empty());
    auto l = s.ls(PP("/a/b"));
    CHK(l.size() == 1 && l[0].name == "c" && l[0].isDir);
    CHK(s.dirSize(PP("/a")) == 23);                       // 11 + 1 + 0 + 11
    CHK(s.dirSize(PP("/nodir")) == 0);
    
    // пути: нормализация и отказ на неверных
    CHK(s.write(PP("/a/p1.txt"), "p"));
    std::string rel  = std::string(root + 1) + "/a/p1.txt";        // без ведущего '/'
    std::string dbl  = std::string(root) + "//a///p1.txt";
    std::string tail = std::string(root) + "/a/";
    std::string dots = std::string(root) + "/a/../a/p1.txt";
    CHK(s.exists(rel.c_str()));
    CHK(s.read(dbl.c_str()) == "p");
    CHK(s.isDir(tail.c_str()));
    CHK(!s.exists(""));
    CHK(!s.mkdir(""));
    CHK(!s.write("", "x"));
    CHK(s.read("").length() == 0);
    CHK(s.ls("").empty());
    CHK(s.find("", "p1").empty());
    CHK(s.dirSize("") == 0);
    CHK(!s.removeAll(""));
    CHK(!s.exists(dots.c_str()));
    CHK(s.read(dots.c_str()).length() == 0);
    CHK(!s.remove(dots.c_str()));
    CHK(!s.rename(PP("/a/p1.txt"), ""));
    CHK(!s.copy(PP("/a/p1.txt"), ""));
    CHK(s.exists(PP("/a/p1.txt")));                     // файл на месте
    CHK(s.remove(PP("/a/p1.txt")));                     // убрать, чтобы не сбить dirSize ниже
    
    // backend-слой: порядок, remove на папке, лимиты глубины
    CHK(s.mkdir(PP("/ord")));
    CHK(s.write(PP("/ord/z1.txt"), "z"));
    CHK(s.write(PP("/ord/a1.txt"), "a"));
    CHK(s.write(PP("/ord/m1.txt"), "m"));
    auto fo = s.find(PP("/ord"), "1");
    CHK(fo.size() == 3 && fo[0] == PP("/ord/a1.txt") && fo[1] == PP("/ord/m1.txt") && fo[2] == PP("/ord/z1.txt"));
    auto lo = s.ls(PP("/ord"));
    CHK(lo.size() == 3 && lo[0].name == "a1.txt" && lo[1].name == "m1.txt" && lo[2].name == "z1.txt");
    CHK(!s.remove(PP("/a/b")));                          // папку remove не удаляет
    CHK(s.isDir(PP("/a/b")));
    CHK(!s.remove(PP("/a/missing.txt")));
    CHK(s.mkdir(PP("/emptyd")));
    CHK(!s.remove(PP("/emptyd")));                       // и пустую тоже
    CHK(s.isDir(PP("/emptyd")));
    CHK(s.removeAll(PP("/emptyd")));
    {   // создание цепочки за один вызов: MAX_DEPTH+1 новых уровней можно, +2 нельзя
        std::string ok = std::string(root) + "/mk1";
        for (int i = 0; i < PDA2_FS_MAX_DEPTH; ++i) ok += "/n" + std::to_string(i);
        std::string bad = std::string(root) + "/mk2";
        for (int i = 0; i <= PDA2_FS_MAX_DEPTH; ++i) bad += "/n" + std::to_string(i);
        CHK(s.mkdir(ok.c_str()));
        CHK(!s.mkdir(bad.c_str()));
        CHK(!s.exists(PP("/mk2")));                      // при отказе ничего не создано
    }
    {   // removeAll глубже лимита отказывает; хвост убирается с более глубокого старта
        std::string dp = std::string(root) + "/deep";
        CHK(s.mkdir(dp.c_str()));
        std::string tail4;
        for (int i = 1; i <= PDA2_FS_MAX_DEPTH + 3; ++i) {
            dp += "/e" + std::to_string(i);
            CHK(s.mkdir(dp.c_str()));
            if (i == 4) tail4 = dp;
        }
        CHK(!s.removeAll(PP("/deep")));
        CHK(s.exists(PP("/deep")));
        CHK(s.removeAll(tail4.c_str()));
        CHK(s.removeAll(PP("/deep")));
    }
    CHK(s.removeAll(PP("/mk1")));
    CHK(s.removeAll(PP("/ord")));

        // пробелы покрытия: границы кусков (512), writeBytes/readBytes с нулевыми байтами, дозапись, rename
    CHK(s.mkdir(PP("/big")));
    {
        const size_t sizes[] = {0, 1, 511, 512, 513, 1024, 1300};
        for (size_t n : sizes) {
            std::vector<uint8_t> pat(n), back(n + 1, 0xEE);
            for (size_t i = 0; i < n; ++i) pat[i] = (uint8_t)(i * 31);   // есть нулевые байты
            const std::string f1 = std::string(root) + "/big/f" + std::to_string(n);
            const std::string f2 = f1 + ".copy";
            CHK(s.writeBytes(f1.c_str(), pat.data(), n));
            CHK(s.size(f1.c_str()) == n);
            CHK(s.readBytes(f1.c_str(), back.data(), n));
            CHK(n == 0 || memcmp(back.data(), pat.data(), n) == 0);
            CHK(s.copy(f1.c_str(), f2.c_str()));
            CHK(s.size(f2.c_str()) == n);
            std::fill(back.begin(), back.end(), 0xEE);
            CHK(s.readBytes(f2.c_str(), back.data(), n));
            CHK(n == 0 || memcmp(back.data(), pat.data(), n) == 0);
            String str = s.read(f2.c_str());
            CHK(str.length() == n);
            CHK(n == 0 || memcmp(str.c_str(), pat.data(), n) == 0);
            CHK(!s.readBytes(f1.c_str(), back.data(), n + 1));           // просим больше, чем есть
        }
    }
    {
        const std::string a(600, 'a'), b(600, 'b');
        const std::string fapp = std::string(root) + "/big/app.txt";
        CHK(s.write(fapp.c_str(), String(a.c_str())));
        CHK(s.append(fapp.c_str(), String(b.c_str())));                  // дозапись через границу куска
        String rapp = s.read(fapp.c_str());
        CHK(rapp.length() == 1200);
        CHK(memcmp(rapp.c_str(), a.c_str(), 600) == 0 && memcmp(rapp.c_str() + 600, b.c_str(), 600) == 0);
    }

        // замер памяти: read() большого файла (только лог, в зачёт не идёт)
    {
        const size_t kBlock = 4096, kBlocks = 64;                       // 256 КБ
        const std::string fm = std::string(root) + "/memprobe.bin";
        const std::string blk(kBlock, 'm');
        bool wrote = true;
        for (size_t i = 0; i < kBlocks && wrote; ++i) wrote = s.append(fm.c_str(), String(blk.c_str()));
        const uint32_t h0 = pda2_platform_free_heap(), p0 = pda2_platform_free_psram();
        {
            String big = s.read(fm.c_str());
            PDA_LOGI("fstest", "[%s] memprobe: записано=%d, read() вернул %u из %u байт; heap %u -> %u, psram %u -> %u",
                     label, (int)wrote, (unsigned)big.length(), (unsigned)(kBlock * kBlocks),
                     (unsigned)h0, (unsigned)pda2_platform_free_heap(),
                     (unsigned)p0, (unsigned)pda2_platform_free_psram());
        }
        PDA_LOGI("fstest", "[%s] memprobe: после освобождения heap %u, psram %u", label,
                 (unsigned)pda2_platform_free_heap(), (unsigned)pda2_platform_free_psram());
        s.remove(fm.c_str());
    }

    CHK(s.write(PP("/big/r1.txt"), "r1"));
    CHK(s.rename(PP("/big/r1.txt"), PP("/big/r2.txt")));
    CHK(!s.exists(PP("/big/r1.txt")));
    CHK(s.read(PP("/big/r2.txt")) == "r1");
    CHK(s.mkdir(PP("/big/rd/sub")));
    CHK(s.write(PP("/big/rd/sub/x.txt"), "x"));
    CHK(s.rename(PP("/big/rd"), PP("/big/rd2")));                        // папка с содержимым
    CHK(!s.exists(PP("/big/rd")));
    CHK(s.read(PP("/big/rd2/sub/x.txt")) == "x");
    CHK(!s.rename(PP("/big/missing"), PP("/big/m3")));
    CHK(!s.rename(PP("/big/r2.txt"), PP("/nodir2/x.txt")));              // назначение в несуществующей папке
    CHK(s.exists(PP("/big/r2.txt")));
    CHK(!s.readBytes(PP("/big/missing"), nullptr, 0));
    CHK(s.removeAll(PP("/big")));

        // шаг 1: FsResult, read(path, out) с лимитом, readBytes со смещением, FsStream
    {
        FsVolume& v = static_cast<FsVolume&>(s);
        String out;
        CHK(s.mkdir(PP("/res")));
        CHK(s.read(PP("/res/none"), out) == FsResult::NotFound);
        CHK(s.read(PP("/res"), out) == FsResult::IsDir);
        CHK(s.write(PP("/res/e.txt"), ""));
        CHK(s.read(PP("/res/e.txt"), out) == FsResult::Ok && out.length() == 0);
        CHK(s.write(PP("/res/a.txt"), "0123456789"));
        v.setReadLimit(5);
        out = "x";
        CHK(s.read(PP("/res/a.txt"), out) == FsResult::TooBig && out.length() == 0);
        v.setReadLimit(0);
        CHK(s.read(PP("/res/a.txt"), out) == FsResult::Ok && out == "0123456789");

        uint8_t b8[4];
        CHK(s.readBytes(PP("/res/a.txt"), b8, 4, 3) == FsResult::Ok && memcmp(b8, "3456", 4) == 0);
        CHK(s.readBytes(PP("/res/a.txt"), b8, 4, 8) == FsResult::ShortRead);
        CHK(s.readBytes(PP("/res/none"), b8, 4) == FsResult::NotFound);
        CHK(s.mkdir(PP("/res/a.txt")) == FsResult::NotDir);
        CHK(s.write(PP("/res"), "x") == FsResult::IsDir);
        CHK(s.write(PP("/res/nodir/x"), "x") == FsResult::NotFound);
        CHK(s.rename(PP("/res/none"), PP("/res/n2")) == FsResult::NotFound);
        CHK(s.copy(PP("/res/a.txt"), PP("/res/a.txt")) == FsResult::Exists);
        CHK(s.remove(PP("/res")) == FsResult::IsDir);

        std::vector<uint8_t> w(3000), r(100);
        for (size_t i = 0; i < w.size(); ++i) w[i] = (uint8_t)(i * 7);
        size_t got = 0;
        {
            FsStream st;
            CHK(s.open(PP("/res/s.bin"), FsOpenMode::Write, st) == FsResult::Ok);
            CHK(st.write(w.data(), 1000) == FsResult::Ok);
            CHK(st.write(w.data() + 1000, 2000) == FsResult::Ok);
            CHK(st.tell() == 3000);
            CHK(st.close() == FsResult::Ok);
        }
        CHK(s.size(PP("/res/s.bin")) == 3000);
        {
            FsStream rd;
            CHK(s.open(PP("/res/s.bin"), FsOpenMode::Read, rd) == FsResult::Ok);
            CHK(rd.size() == 3000);
            CHK(rd.seek(2900) == FsResult::Ok);
            CHK(rd.read(r.data(), 100, got) == FsResult::Ok && got == 100 && memcmp(r.data(), w.data() + 2900, 100) == 0);
            CHK(rd.read(r.data(), 100, got) == FsResult::Ok && got == 0);          // конец файла
            CHK(rd.seek(0) == FsResult::Ok);                                       // назад после EOF
            CHK(rd.read(r.data(), 10, got) == FsResult::Ok && got == 10 && memcmp(r.data(), w.data(), 10) == 0);
            CHK(rd.write(w.data(), 1) == FsResult::Unsupported);
        }
        {
            FsStream ap;
            CHK(s.open(PP("/res/s.bin"), FsOpenMode::Append, ap) == FsResult::Ok);
            CHK(ap.seek(0) == FsResult::Unsupported);
            CHK(ap.write(w.data(), 10) == FsResult::Ok);
            CHK(ap.close() == FsResult::Ok);
        }
        CHK(s.size(PP("/res/s.bin")) == 3010);
        FsStream none;
        CHK(s.open(PP("/res/none"), FsOpenMode::Read, none) == FsResult::NotFound);
        CHK(s.open(PP("/res"), FsOpenMode::Read, none) == FsResult::IsDir);

        std::vector<String> fr;
        bool tr = true;
        CHK(s.find(PP("/res"), "a.t", fr, &tr) == FsResult::Ok && fr.size() == 1 && !tr);
        CHK(s.removeAll(PP("/res")));
    }

    // кириллица в имени файла: справочно, в зачёт не идёт (зависит от настроек FAT)
    bool cyW = s.write(PP("/a/Привет.txt"), "p");
    bool cyF = cyW && has(s.find(PP("/a"), "Привет"), PP("/a/Привет.txt"));
    PDA_LOGI("fstest", "[%s] кириллица в имени: запись=%d поиск=%d", label, (int)cyW, (int)cyF);

    // 66 файлов: find отдаёт не больше PDA2_FS_FIND_MAX; замеряем время
    CHK(s.mkdir(PP("/many")));
    uint32_t t0 = pda2_platform_now_ms();
    int written = 0;
    for (int i = 0; i < 66; ++i)
        if (s.write(PP(("/many/m" + std::to_string(i) + ".txt").c_str()), "m")) ++written;
    uint32_t tWrite = pda2_platform_now_ms() - t0;
    CHK(written == 66);
    t0 = pda2_platform_now_ms();
    size_t found = s.find(PP("/many"), "m").size();
    uint32_t tFind = pda2_platform_now_ms() - t0;
    CHK(found == (size_t)PDA2_FS_FIND_MAX);

    // глубина: d1..d10, f8 на допустимом уровне виден, f10 — нет
    std::string p = root;
    std::string d6;
    for (int i = 1; i <= 10; ++i) {
        p += "/d" + std::to_string(i);
        CHK(s.mkdir(p.c_str()));
        if (i == 6) d6 = p;
        if (i == 8) CHK(s.write((p + "/f8.txt").c_str(), "8"));
    }
    CHK(s.write((p + "/f10.txt").c_str(), "10"));
    CHK(s.find(root, "f8.txt").size() == 1);
    CHK(s.find(root, "f10.txt").empty());

    // removeAll: несуществующее и вложенное дерево
    CHK(!s.removeAll(PP("/nonexistent")));
    CHK(s.mkdir(PP("/r/r1/r2/r3/r4")));
    CHK(s.write(PP("/r/f0.txt"), "0"));
    CHK(s.write(PP("/r/r1/f1.txt"), "1"));
    CHK(s.write(PP("/r/r1/r2/r3/r4/f4.txt"), "4"));
    CHK(s.removeAll(PP("/r")));
    CHK(!s.exists(PP("/r")));

    // уборка: сначала глубокий хвост (d6..d10), потом вся тестовая папка; замеряем время
    CHK(s.removeAll(d6.c_str()));
    t0 = pda2_platform_now_ms();
    CHK(s.removeAll(root));
    uint32_t tRemove = pda2_platform_now_ms() - t0;
    CHK(!s.exists(root));

    PDA_LOGI("fstest", "[%s] время: запись 66 файлов %u мс, find %u мс, removeAll %u мс", label,
             (unsigned)tWrite, (unsigned)tFind, (unsigned)tRemove);
    PDA_LOGI("fstest", "[%s] готово: %d ок, %d провалов. Свободно: %u МБ", label,
             g_pass - pass0, g_fail - fail0,
             (unsigned)(s.freeSpace() / (1024ULL * 1024ULL)));
}

} // namespace

void fs_selftest(Fs_Class& fs) {
    PDA_LOGI("fstest", "=== самотест Fs (папка /_fstest) ===");
    run(fs.internal, "LittleFS");
    run(fs.sd,       "SD");
    PDA_LOGI("fstest", "=== ИТОГ: %d ок, %d провалов ===", g_pass, g_fail);
}

#endif // PDA2_FS_SELFTEST