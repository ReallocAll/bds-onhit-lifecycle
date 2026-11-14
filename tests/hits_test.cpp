#include "hits.hpp"
#include <cassert>

int main() {
    using onhit::chunk_start;
    assert(chunk_start(0) == 0);
    assert(chunk_start(15) == 0);
    assert(chunk_start(16) == 16);
    assert(chunk_start(-1) == -16);
    assert(chunk_start(-16) == -16);
    assert(chunk_start(-17) == -32);
    onhit::HitChunks<3> chunks;
    chunks.record("OW", -16, 16);
    chunks.record("NE", -16, 16);
    chunks.record("OW", -16, 16);
    auto rows = chunks.top(20);
    assert(rows.size() == 2 && rows[0].dim == "OW" && rows[0].count == 2);
    chunks.record("END", 0, 0);
    chunks.record("OW", 16, 16);
    assert(chunks.size() == 3);
    rows = chunks.top(20);
    assert(rows.size() == 3 && rows[0].count >= rows[1].count);
    assert(chunks.top(2).size() == 2);
}
