// perf_logic.h
//
// PURE performance models for fix thse isseue.md item 7, mirrored into
// ctrlg-group.wh.cpp:
//   * autogroupbucket:: -- build the Auto Group file -> bucket assignment in ONE
//     pass (a normalized-key map) instead of rescanning plan.files per bucket
//     (the old O(buckets x files) loop, 13636-13750).
//   * thumbcache::      -- request DEDUPLICATION + BOUNDED caching for the async
//     image-thumbnail cache (evict the oldest COMPLETED entry past the cap).
//
// Exercised by tests\perf_test.cpp.
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
namespace autogroupbucket {

// Case- and separator-insensitive directory key. Mirrors autogroup::PathEqualCI:
// A-Z -> a-z and '/' -> '\\', so two paths that PathEqualCI considers equal map
// to the SAME key. Equal keys <=> same destination bucket.
inline std::wstring NormalizeDirKey(const std::wstring& dir) {
    std::wstring k;
    k.reserve(dir.size());
    for (wchar_t c : dir) {
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        else if (c == L'/')         c = L'\\';
        k.push_back(c);
    }
    return k;
}

// Assign each file (by index, in input order) to the bucket whose normalized
// key matches its destination directory. Built in ONE pass over the files using
// a key->bucketIndex map, so the whole routing is O(files + buckets) instead of
// O(buckets x files). The result is aligned to `buckets` and is identical to a
// brute-force PathEqualCI filter when the buckets are distinct (which the Auto
// Group plan guarantees). A file whose destDir matches no bucket is dropped,
// exactly as the brute-force loop did.
inline std::vector<std::vector<int>> AssignFilesToBuckets(
        const std::vector<std::wstring>& fileDestDirs,
        const std::vector<std::wstring>& buckets) {
    std::vector<std::vector<int>> out(buckets.size());
    std::unordered_map<std::wstring, int> keyToBucket;
    keyToBucket.reserve(buckets.size() * 2);
    for (int b = 0; b < static_cast<int>(buckets.size()); ++b) {
        // First occurrence wins (distinct buckets -> exactly one mapping each).
        keyToBucket.emplace(NormalizeDirKey(buckets[b]), b);
    }
    for (int f = 0; f < static_cast<int>(fileDestDirs.size()); ++f) {
        auto it = keyToBucket.find(NormalizeDirKey(fileDestDirs[f]));
        if (it != keyToBucket.end()) {
            out[it->second].push_back(f);
        }
    }
    return out;
}

}  // namespace autogroupbucket

// ---------------------------------------------------------------------------
namespace thumbcache {

// Upper bound on cached thumbnail entries. Scrolling a large image selection
// used to grow the cache (and its GDI bitmaps) without limit; past this many
// entries the oldest COMPLETED entry is evicted and its bitmap deleted.
inline int MaxEntries() { return 128; }

// Evict once the cache has grown PAST the cap.
inline bool ShouldEvict(int cacheSize, int maxEntries) {
    return cacheSize > maxEntries;
}

// Request dedup: a key already present in the cache (completed OR in-flight)
// must NOT be enqueued again, so the worker never does duplicate extraction.
inline bool ShouldEnqueue(bool keyAlreadyPresent) {
    return !keyAlreadyPresent;
}

// Choose which entry to evict: the OLDEST (lowest index) COMPLETED entry.
// An in-flight entry (done == false) is NEVER evicted -- the worker thread may
// still be writing its result, and evicting it would leak or corrupt. Returns
// -1 when nothing is safely evictable.
inline int PickEvictIndex(const std::vector<bool>& done) {
    for (int i = 0; i < static_cast<int>(done.size()); ++i) {
        if (done[i]) return i;
    }
    return -1;
}

}  // namespace thumbcache
