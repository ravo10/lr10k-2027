// Unit tests for TestData (main/LloydLR10KTensile/TestData.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/test_data_test.cpp -o test_data_test
//   ./test_data_test
//
// Run automatically by .github/workflows/tests.yml.

#include "TestData.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define NEAR(a, b, tol) CHECK(std::fabs((double)(a) - (double)(b)) <= (tol))

using namespace TestData;

// Like ImVec2 (float) and ImPlotPoint (double)
struct Vec2 { float x = 0, y = 0; };
struct PlotPoint { double x = 0, y = 0; };
using Curve = std::vector<Vec2>;
using Points = std::vector<std::tuple<PlotPoint, int>>;

static Curve Ramp(int n, float slope = 2.0f)
{
    Curve c;
    for (int i = 0; i < n; ++i) c.push_back({ 0.1f * i, slope * i });
    return c;
}

// The old SmoothTensileData() from Calculator.cpp, unchanged, to compare with
static Curve OldSmooth(const Curve& raw, int window)
{
    if ((int)raw.size() < window || window < 2) return raw;
    Curve out = raw;
    int half = window / 2;
    for (int i = half; i < (int)raw.size() - half; ++i)
    {
        double sum = 0.0;
        for (int j = -half; j <= half; ++j) sum += raw[i + j].y;
        out[i].y = (float)(sum / window);
    }
    return out;
}

static void test_smoothing_keeps_a_constant_load()
{
    // A constant load must stay the same with any window (the old code: 50 % too high with a window of 2)
    Curve flat;
    for (int i = 0; i < 100; ++i) flat.push_back({ 0.1f * i, 500.0f });
    for (int window = 1; window <= 51; ++window)
    {
        const Curve s = MovingAverageY(flat, window);
        bool same = true;
        for (const Vec2& p : s) same &= std::fabs(p.y - 500.0f) < 1e-3f;
        if (!same) std::printf("  window %d changed a constant load\n", window);
        CHECK(same);
    }
    CHECK(std::fabs(OldSmooth(flat, 2)[50].y - 750.0f) < 1e-3f); // The bug: +50 %
    CHECK(std::fabs(OldSmooth(flat, 4)[50].y - 625.0f) < 1e-3f); // +25 %
}

static void test_smoothing_odd_window_is_unchanged()
{
    // Odd windows (the default 5) give exactly the old values: saved tests look the same
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> noise(-3.0f, 3.0f);
    Curve c = Ramp(300);
    for (Vec2& p : c) p.y += noise(rng);
    for (int window = 1; window <= 51; window += 2)
    {
        const Curve a = MovingAverageY(c, window), b = OldSmooth(c, window);
        bool same = a.size() == b.size();
        for (std::size_t i = 0; same && i < a.size(); ++i) same = a[i].x == b[i].x && a[i].y == b[i].y;
        CHECK(same);
    }
}

static void test_smoothing_edges_and_short_curves()
{
    const Curve c = Ramp(10);
    const Curve s = MovingAverageY(c, 5);
    CHECK(s[0].y == c[0].y && s[1].y == c[1].y && s[9].y == c[9].y); // No full window: kept
    NEAR(s[5].y, c[5].y, 1e-4);   // A straight line stays straight
    CHECK(MovingAverageY(Ramp(3), 5).size() == 3);
    CHECK(MovingAverageY(Curve{}, 5).empty());
}

static void test_closest_and_snap()
{
    const Curve c = Ramp(11); // x = 0.0, 0.1, ... 1.0
    CHECK(ClosestIndexByX(Curve{}, 1.0) == -1);
    CHECK(ClosestIndexByX(c, 0.34) == 3);
    CHECK(ClosestIndexByX(c, -5.0) == 0);
    CHECK(ClosestIndexByX(c, 99.0) == 10);

    Points pts = { { PlotPoint{ 0.52, 0.0 }, 2 }, { PlotPoint{ 0.88, 123.0 }, 3 } };
    SnapToCurve(pts, c);
    NEAR(std::get<0>(pts[0]).x, c[5].x, 1e-9);
    NEAR(std::get<0>(pts[0]).y, c[5].y, 1e-9);
    NEAR(std::get<0>(pts[1]).y, c[9].y, 1e-9);
    CHECK(std::get<1>(pts[1]) == 3); // IDs untouched

    Points unchanged = { { PlotPoint{ 1, 2 }, 0 } };
    SnapToCurve(unchanged, Curve{});
    CHECK(std::get<0>(unchanged[0]).x == 1 && std::get<0>(unchanged[0]).y == 2);
}

static void test_max_load()
{
    CHECK(MaxLoadIndex(Curve{}) == -1);
    CHECK(MaxLoadIndex(Curve{ { 0, -1 }, { 1, 0 } }) == -1); // Nothing above 0 (as the old GetUTS)
    CHECK(MaxLoadIndex(Curve{ { 0, 1 }, { 1, 9 }, { 2, 9 }, { 3, 4 } }) == 1); // The first of equal maxima
}

static void test_accepts_new_point()
{
    CHECK(AcceptsNewPoint(0.0, 0.0, 0.0));     // First point
    CHECK(AcceptsNewPoint(1.0, 1.1, 5.0));
    CHECK(!AcceptsNewPoint(1.0, 1.0, 5.0));    // Same extension
    CHECK(!AcceptsNewPoint(1.0, 0.9, 5.0));    // Going back
    CHECK(!AcceptsNewPoint(0.0, -0.1, 5.0));   // Negative
    CHECK(!AcceptsNewPoint(0.0, 0.1, -5.0));
}

static void test_travel_limit()
{
    CHECK(TravelLimitAction(0.0, 100.0, 0) == TravelAction::None);   // No limit
    CHECK(TravelLimitAction(10.0, 9.99, 0) == TravelAction::None);
    CHECK(TravelLimitAction(10.0, 10.0, 0) == TravelAction::EndTest);
    CHECK(TravelLimitAction(10.0, 12.0, 1) == TravelAction::PauseTest);
    CHECK(TravelLimitAction(10.0, 12.0, 7) == TravelAction::None);
}

static void test_point_list()
{
    Points pts = { { PlotPoint{ 1, 1 }, 2 }, { PlotPoint{ 2, 2 }, 3 }, { PlotPoint{ 3, 3 }, 2 }, { PlotPoint{ 4, 4 }, 5 } };
    CHECK(HasId(pts, 3) && !HasId(pts, 0));
    CHECK(FindId(pts, 2) != nullptr && FindId(pts, 2)->x == 1);
    CHECK(FindId(pts, 9) == nullptr);
    CHECK(FindId(Points{}, 2) == nullptr);

    const Points first = FirstOfEachId(pts);
    CHECK(first.size() == 3);
    CHECK(std::get<0>(first[0]).x == 1 && std::get<1>(first[2]) == 5);

    CHECK(RemoveId(pts, 2) == 2);
    CHECK(pts.size() == 2 && !HasId(pts, 2));
    CHECK(RemoveId(pts, 2) == 0);

    // Also with pairs (GetCrucialDataPoints returns pairs)
    std::vector<std::pair<PlotPoint, int>> pairs = { { PlotPoint{ 1, 1 }, 0 } };
    CHECK(HasId(pairs, 0));

    int lower = 0, upper = 0;
    CHECK(!DefaultEModulusBounds(50, lower, upper));
    CHECK(DefaultEModulusBounds(51, lower, upper) && lower == 10 && upper == 50);
}

int main()
{
    test_smoothing_keeps_a_constant_load();
    test_smoothing_odd_window_is_unchanged();
    test_smoothing_edges_and_short_curves();
    test_closest_and_snap();
    test_max_load();
    test_accepts_new_point();
    test_travel_limit();
    test_point_list();

    if (g_failures == 0)
        std::printf("test_data_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
