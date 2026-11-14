#include "hits.hpp"
#include <cassert>
#include <climits>

int main() {
    using onhit::chunk_start;
    assert(chunk_start(0) == 0);
    assert(chunk_start(15) == 0);
    assert(chunk_start(16) == 16);
    assert(chunk_start(-1) == -16);
    assert(chunk_start(-16) == -16);
    assert(chunk_start(-17) == -32);
    assert(chunk_start(INT_MIN) == INT_MIN);

    onhit::HitChunks<3> chunks;
    chunks.record("OW", -16, 16);
    chunks.record("NE", -16, 16);
    chunks.record("OW", -16, 16);
    auto rows = chunks.top(20);
    assert(rows.size() == 2 && rows[0].dim == "OW" && rows[0].count == 2);
    assert(rows[1].dim == "NE" && rows[1].count == 1);
    chunks.record("END", 0, 0);
    chunks.record("OW", 16, 16);
    assert(chunks.size() == 3);
    rows = chunks.top(20);
    assert(rows.size() == 3 && rows[0].count >= rows[1].count);
    assert(chunks.top(2).size() == 2);

    onhit::HitChunks<128> populated;
    for (int i = 0; i < 25; ++i) {
        for (int j = 0; j <= i; ++j) populated.record("OW", i * 16, 0);
    }
    auto top = populated.top(20);
    assert(top.size() == 20);
    assert(top.front().x == 24 * 16 && top.front().count == 25);
    assert(top.back().count == 6);
    for (int i = 1; i < 20; ++i) assert(top[i-1].count >= top[i].count);
    populated.record("NE", 0, 0);
    populated.record("END", 0, 0);
    assert(populated.size() == 27);
}
