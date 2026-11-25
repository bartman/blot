/* blot: helpers shared by the unit tests (#2)
 *
 * The name must not match test_*.cpp: tests/CMakeLists.txt turns every
 * such file into its own executable.
 *
 * Conventions (see the test design on issue #2):
 * - always call blot_figure_set_screen_size(), never rely on the terminal;
 * - use F (NO_COLOR | NO_UNICODE) unless the test is about colour,
 *   unicode or braille;
 * - goldens are compared right-trimmed, one std::wstring per line;
 * - plot rows and canvas rows are numbered from 0 at the bottom, unless
 *   BLOT_RENDER_DONT_INVERT_Y_AXIS is set;
 * - a test that may hang, crash or touch process-wide state (the cached
 *   terminal size, have_color_support, the environment) runs in
 *   RUN_ISOLATED.
 */
#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>

#include "blot_figure.h"
#include "blot_error.h"
#include "blot_types.h"
#include "blot_screen.h"
#include "blot_layer.h"
#include "blot_canvas.h"
#include "blot_axis.h"
#include "blot_color.h"
#include "blot_terminal.h"

namespace blot_test {

using Lines = std::vector<std::wstring>;
using Cell = std::pair<unsigned, unsigned>; // (col, row)
using Cells = std::set<Cell>;

constexpr blot_render_flags F = BLOT_RENDER_NO_COLOR | BLOT_RENDER_NO_UNICODE;

/* --- text ------------------------------------------------------------ */

/* UTF-8 for gtest messages; ESC is shown as \e */
static inline std::string to_utf8(const std::wstring &w)
{
    std::string s;
    for (wchar_t wc : w) {
        unsigned c = (unsigned)wc;
        if (c == 0x1b) {
            s += "\\e";
        } else if (c < 0x80) {
            s += (char)c;
        } else if (c < 0x800) {
            s += (char)(0xc0 | (c >> 6));
            s += (char)(0x80 | (c & 0x3f));
        } else if (c < 0x10000) {
            s += (char)(0xe0 | (c >> 12));
            s += (char)(0x80 | ((c >> 6) & 0x3f));
            s += (char)(0x80 | (c & 0x3f));
        } else {
            s += (char)(0xf0 | (c >> 18));
            s += (char)(0x80 | ((c >> 12) & 0x3f));
            s += (char)(0x80 | ((c >> 6) & 0x3f));
            s += (char)(0x80 | (c & 0x3f));
        }
    }
    return s;
}

/* lines without their '\n'; the text ends with one, which adds no line */
static inline Lines split_lines(const std::wstring &text)
{
    Lines out;
    size_t start = 0;
    while (start < text.size()) {
        size_t nl = text.find(L'\n', start);
        if (nl == std::wstring::npos) {
            out.push_back(text.substr(start));
            break;
        }
        out.push_back(text.substr(start, nl - start));
        start = nl + 1;
    }
    return out;
}

static inline std::wstring rtrim(const std::wstring &s)
{
    size_t end = s.find_last_not_of(L' ');
    return end == std::wstring::npos ? std::wstring() : s.substr(0, end + 1);
}

static inline Lines rtrim(const Lines &lines)
{
    Lines out;
    for (const auto &l : lines)
        out.push_back(rtrim(l));
    return out;
}

static inline std::wstring trim(const std::wstring &s)
{
    size_t b = s.find_first_not_of(L' ');
    if (b == std::wstring::npos)
        return std::wstring();
    return s.substr(b, s.find_last_not_of(L' ') - b + 1);
}

/* remove every SGR sequence (ESC [ ... m) */
static inline std::wstring strip_sgr(const std::wstring &s)
{
    std::wstring out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == 0x1b && i + 1 < s.size() && s[i + 1] == L'[') {
            size_t j = i + 2;
            while (j < s.size() && (iswdigit(s[j]) || s[j] == L';'))
                j++;
            if (j < s.size() && s[j] == L'm') {
                i = j;
                continue;
            }
        }
        out += s[i];
    }
    return out;
}

static inline Lines strip_sgr(const Lines &lines)
{
    Lines out;
    for (const auto &l : lines)
        out.push_back(strip_sgr(l));
    return out;
}

static inline bool has_esc(const std::wstring &s)
{
    return s.find(L'\x1b') != std::wstring::npos;
}

/* both blocks with line numbers; a '!' marks the lines that differ */
static inline ::testing::AssertionResult lines_eq(const Lines &actual, const Lines &expected)
{
    if (actual == expected)
        return ::testing::AssertionSuccess();

    std::ostringstream os;
    os << "rendered lines differ (" << actual.size() << " actual, "
       << expected.size() << " expected)\n";
    size_t n = std::max(actual.size(), expected.size());
    os << "expected:\n";
    for (size_t i = 0; i < expected.size(); i++)
        os << (i < actual.size() && actual[i] == expected[i] ? "  " : "! ")
           << (i < 10 ? " " : "") << i << " |" << to_utf8(expected[i]) << "|\n";
    os << "actual:\n";
    for (size_t i = 0; i < n && i < actual.size(); i++)
        os << (i < expected.size() && actual[i] == expected[i] ? "  " : "! ")
           << (i < 10 ? " " : "") << i << " |" << to_utf8(actual[i]) << "|\n";
    return ::testing::AssertionFailure() << os.str();
}

#define EXPECT_LINES_EQ(actual, expected) EXPECT_TRUE(::blot_test::lines_eq((actual), (expected)))

/* --- figures --------------------------------------------------------- */

/* a figure that is deleted at the end of the scope */
struct Fig {
    blot_figure *fig;

    Fig() : fig(blot_figure_new(NULL)) {}
    Fig(unsigned cols, unsigned rows) : Fig() { size(cols, rows); }
    ~Fig() { blot_figure_delete(fig); }
    Fig(const Fig &) = delete;
    Fig &operator=(const Fig &) = delete;

    operator blot_figure *() const { return fig; }

    void size(unsigned cols, unsigned rows)
    {
        EXPECT_TRUE(blot_figure_set_screen_size(fig, cols, rows, NULL));
    }
    void xlim(double lo, double hi) { EXPECT_TRUE(blot_figure_set_x_limits(fig, lo, hi, NULL)); }
    void ylim(double lo, double hi) { EXPECT_TRUE(blot_figure_set_y_limits(fig, lo, hi, NULL)); }
    void lim(double xlo, double xhi, double ylo, double yhi) { xlim(xlo, xhi); ylim(ylo, yhi); }

    /* the figure keeps pointers to xs and ys: they must outlive render() */
    void plot(blot_plot_type t, blot_data_type dt, size_t n, const void *xs, const void *ys,
              blot_color color = 1, const char *label = NULL)
    {
        GError *error = NULL;
        EXPECT_TRUE(blot_figure_plot(fig, t, dt, n, xs, ys, color, label, &error));
        if (error)
            ADD_FAILURE() << error->message;
        g_clear_error(&error);
    }
};

struct Rendered {
    bool ok = false;
    int code = 0;
    std::string message;
    blot_render_flags flags = BLOT_RENDER_NONE;
    std::wstring text;
    Lines lines;            // as rendered, without the '\n'
    blot_dimensions dim = {};
    blot_margins mrg = {};

    /* plot area size */
    unsigned plot_cols() const { return dim.cols - mrg.left - mrg.right; }
    unsigned plot_rows() const { return dim.rows - mrg.top - mrg.bottom; }
};

static inline Rendered render(blot_figure *fig, blot_render_flags flags)
{
    Rendered r;
    GError *error = NULL;

    r.flags = flags;
    blot_screen *scr = blot_figure_render(fig, flags, &error);
    if (!scr) {
        r.ok = false;
        r.code = error ? error->code : 0;
        r.message = error ? error->message : "(no error)";
        g_clear_error(&error);
        return r;
    }
    EXPECT_TRUE(error == NULL);
    g_clear_error(&error);

    gsize size = 0;
    const wchar_t *txt = blot_screen_get_text(scr, &size, NULL);
    r.ok = true;
    r.text.assign(txt, size);
    r.lines = split_lines(r.text);
    r.dim = scr->dim;
    r.mrg = scr->mrg;
    blot_screen_delete(scr);
    return r;
}

/* the plot area as a set of marks (col, row); row 0 is the bottom unless
 * DONT_INVERT_Y_AXIS.  Only for renders without a legend or CLEAR. */
static inline Cells plot_cells(const Rendered &r)
{
    Cells out;
    if (!r.ok)
        return out;
    Lines lines = strip_sgr(r.lines);
    bool invert = !(r.flags & BLOT_RENDER_DONT_INVERT_Y_AXIS);
    unsigned top = r.mrg.top, hgt = r.plot_rows();
    for (unsigned sy = top; sy < top + hgt && sy < lines.size(); sy++) {
        unsigned row = invert ? hgt - 1 - (sy - top) : sy - top;
        const std::wstring &l = lines[sy];
        for (unsigned sx = r.mrg.left; sx < r.dim.cols - r.mrg.right && sx < l.size(); sx++)
            if (l[sx] != L' ')
                out.insert({sx - r.mrg.left, row});
    }
    return out;
}

/* {row, label} for the Y-axis rows whose axis character is '*' */
static inline std::vector<std::pair<unsigned, std::string>> y_ticks(const Rendered &r)
{
    std::vector<std::pair<unsigned, std::string>> out;
    if (!r.ok || r.mrg.left < 2)
        return out;
    Lines lines = strip_sgr(r.lines);
    bool invert = !(r.flags & BLOT_RENDER_DONT_INVERT_Y_AXIS);
    unsigned top = r.mrg.top, hgt = r.plot_rows();
    for (unsigned sy = top; sy < top + hgt && sy < lines.size(); sy++) {
        const std::wstring &l = lines[sy];
        if (l.size() < r.mrg.left || l[r.mrg.left - 1] != L'*')
            continue;
        unsigned row = invert ? hgt - 1 - (sy - top) : sy - top;
        out.push_back({row, to_utf8(trim(l.substr(0, r.mrg.left - 2)))});
    }
    std::sort(out.begin(), out.end());
    return out;
}

/* {col, label} for the X-axis ticks ('*' on the axis line) */
static inline std::vector<std::pair<unsigned, std::string>> x_ticks(const Rendered &r)
{
    std::vector<std::pair<unsigned, std::string>> out;
    if (!r.ok || r.mrg.bottom < 2)
        return out;
    Lines lines = strip_sgr(r.lines);
    unsigned ax = r.mrg.top + r.plot_rows();
    if (ax + 1 >= lines.size())
        return out;
    const std::wstring &axis = lines[ax], &labels = lines[ax + 1];
    for (unsigned sx = r.mrg.left; sx < axis.size() && sx < r.dim.cols - r.mrg.right; sx++) {
        if (axis[sx] != L'*')
            continue;
        std::wstring label;
        for (unsigned i = sx; i < labels.size() && labels[i] != L' '; i++)
            label += labels[i];
        out.push_back({sx - r.mrg.left, to_utf8(label)});
    }
    return out;
}

static inline std::vector<std::string> labels_of(const std::vector<std::pair<unsigned, std::string>> &ticks)
{
    std::vector<std::string> out;
    for (const auto &t : ticks)
        out.push_back(t.second);
    return out;
}

/* the cell a value should land in: the mapping the axis labels use */
static inline unsigned expected_cell(double v, double min, double max, unsigned len)
{
    double c = std::floor((v - min) * len / (max - min));
    return (unsigned)std::min<double>(len - 1, c);
}

/* --- FIX-A ------------------------------------------------------------
 *
 * Screen 28x12, x limits 0..19, y limits 0..9: with flags F that is
 * mrg.left 8, mrg.bottom 2 and a 20x10 plot area with one cell per
 * integer.  SC is the scatter data (INT32, colour 1, label "pts"). */

inline const gint32 SC_X[] = { 0, 19, 10, 5 };
inline const gint32 SC_Y[] = { 0,  9,  5, 2 };

static inline void fix_a(Fig &f)
{
    f.size(28, 12);
    f.lim(0, 19, 0, 9);
}

static inline void fix_a_scatter(Fig &f)
{
    fix_a(f);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 4, SC_X, SC_Y, 1, "pts");
}

/* FIX-A scatter SC with flags F */
inline const Lines G1 = {
    L"       |                   *",
    L"       |",
    L"       |",
    L" 6.000 *",
    L"       |          *",
    L"       |",
    L" 3.000 *",
    L"       |     *",
    L"       |",
    L" 0.000 **",
    L"       +*-----*------*------",
    L"        0.000 6.333  12.67",
};

/* --- canvases -------------------------------------------------------- */

/* every set dot of a canvas, in canvas coordinates (dots for braille) */
static inline Cells cells(const blot_canvas *can)
{
    Cells out;
    for (unsigned r = 0; r < can->dim.rows; r++)
        for (unsigned c = 0; c < can->dim.cols; c++)
            if (blot_canvas_get(can, c, r))
                out.insert({c, r});
    return out;
}

/* every non-empty glyph cell of a braille canvas */
static inline std::map<Cell, wchar_t> glyphs(const blot_canvas *can)
{
    std::map<Cell, wchar_t> out;
    unsigned cols = can->dim.cols, rows = can->dim.rows;
    if (can->flags & BLOT_RENDER_BRAILLE) {
        cols /= BRAILLE_GLYPH_COLS;
        rows /= BRAILLE_GLYPH_ROWS;
    }
    for (unsigned r = 0; r < rows; r++)
        for (unsigned c = 0; c < cols; c++)
            if (wchar_t w = blot_canvas_get_cell(can, c, r))
                out[{c, r}] = w;
    return out;
}

static inline std::string cells_str(const Cells &cs)
{
    std::ostringstream os;
    for (const auto &c : cs)
        os << " (" << c.first << "," << c.second << ")";
    return os.str();
}

static inline ::testing::AssertionResult cells_eq(const Cells &actual, const Cells &expected)
{
    if (actual == expected)
        return ::testing::AssertionSuccess();
    Cells missing, extra;
    std::set_difference(expected.begin(), expected.end(), actual.begin(), actual.end(),
                        std::inserter(missing, missing.begin()));
    std::set_difference(actual.begin(), actual.end(), expected.begin(), expected.end(),
                        std::inserter(extra, extra.begin()));
    return ::testing::AssertionFailure()
        << "cells differ\n  missing:" << cells_str(missing)
        << "\n  extra:  " << cells_str(extra)
        << "\n  actual: " << cells_str(actual);
}

#define EXPECT_CELLS_EQ(actual, expected) EXPECT_TRUE(::blot_test::cells_eq((actual), (expected)))

/* a canvas that is deleted at the end of the scope */
struct Canvas {
    blot_canvas *can;
    Canvas(unsigned cols, unsigned rows, blot_render_flags flags = BLOT_RENDER_NO_UNICODE)
        : can(blot_canvas_new(cols, rows, flags, 1, NULL)) {}
    explicit Canvas(blot_canvas *c) : can(c) {}
    ~Canvas() { if (can) blot_canvas_delete(can); }
    Canvas(const Canvas &) = delete;
    Canvas &operator=(const Canvas &) = delete;
    operator blot_canvas *() const { return can; }
    blot_canvas *operator->() const { return can; }
};

/* --- isolation ------------------------------------------------------- */

/* Run the body in a forked child with an alarm: a hang fails with SIGALRM
 * after `seconds`, a crash is a normal test failure, and process-wide
 * state (cached terminal size, have_color_support, the environment)
 * can't leak into other tests.  The child exits 0 only when the body
 * recorded no failure of its own. */
#ifdef BLOT_TEST_GCOV
/* a --coverage build (tests/CMakeLists.txt): the child writes its
 * counters before _exit() */
extern "C" void __gcov_dump(void);
#endif

template <typename Fn>
[[noreturn]] void isolated_child(unsigned seconds, Fn body)
{
    /* only the failures of this child count, not those the parent
     * recorded before the fork (an earlier case of the same test) */
    const ::testing::TestInfo *ti = ::testing::UnitTest::GetInstance()->current_test_info();
    int before = ti ? ti->result()->total_part_count() : 0;
    alarm(seconds);
    body();
    /* the parent shows the child's stderr: repeat the failures there */
    bool failed = false;
    if (ti) {
        const ::testing::TestResult *res = ti->result();
        for (int i = before; i < res->total_part_count(); i++) {
            const ::testing::TestPartResult &p = res->GetTestPartResult(i);
            failed |= p.failed();
            if (p.failed())
                fprintf(stderr, "%s:%d: %s\n", p.file_name() ? p.file_name() : "?",
                        p.line_number(), p.message());
        }
    }
    fflush(stdout);
    fflush(stderr);
#ifdef BLOT_TEST_GCOV
    __gcov_dump();
#endif
    _exit(failed ? 1 : 0);
}

/* RUN_ISOLATED(seconds, { body }); the body may use EXPECT_* and ASSERT_* */
#define RUN_ISOLATED(seconds, ...)                                              \
    do {                                                                        \
        auto _isolated_body = [&]() __VA_ARGS__;                                \
        EXPECT_EXIT(::blot_test::isolated_child((seconds), _isolated_body),     \
                    ::testing::ExitedWithCode(0), "");                          \
    } while (0)

} // namespace blot_test
