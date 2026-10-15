// perf_test.cpp
// Pins the fix thse isseue.md item 7 performance findings as PURE models the
// mod mirrors:
//
//   * P2 13636-13750  Auto Group looped over every planned bucket and rescanned
//     ALL plan.files to build each bucket -> O(buckets x files). The file ->
//     bucket assignment must be built in ONE pass (a normalized-key map) and
//     produce exactly the same routing as the old brute-force filter.
//   * Thumbnail cache: requests must DEDUPLICATE (a key already present is not
//     re-enqueued) and the cache must be BOUNDED (evict the oldest COMPLETED
//     entry once it exceeds the cap; never evict an in-flight entry the worker
//     may still be writing).
//
// Self-contained: own main(), exit 0 = pass.

#include "perf_logic.h"

#include <string>
#include <vector>
#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Brute-force O(buckets x files) reference the optimized map must match.
static std::vector<std::vector<int>> BruteForceAssign(
        const std::vector<std::wstring>& fileDestDirs,
        const std::vector<std::wstring>& buckets) {
    std::vector<std::vector<int>> out(buckets.size());
    for (size_t b = 0; b < buckets.size(); ++b) {
        for (size_t f = 0; f < fileDestDirs.size(); ++f) {
            if (autogroupbucket::NormalizeDirKey(fileDestDirs[f]) ==
                autogroupbucket::NormalizeDirKey(buckets[b])) {
                out[b].push_back((int)f);
            }
        }
    }
    return out;
}

int main() {
    // ---- autogroupbucket: one-pass assignment == brute force --------------
    using autogroupbucket::NormalizeDirKey;
    using autogroupbucket::AssignFilesToBuckets;

    // case + separator insensitivity (mirror of autogroup::PathEqualCI)
    check(NormalizeDirKey(L"C:\\Root\\Images") == NormalizeDirKey(L"c:/root/images"),
          "dir key is case- and separator-insensitive");
    check(NormalizeDirKey(L"C:\\a") != NormalizeDirKey(L"C:\\b"),
          "different dirs have different keys");

    std::vector<std::wstring> buckets = {
        L"C:\\root\\Images", L"C:\\root\\Docs", L"C:\\root\\Audio",
    };
    std::vector<std::wstring> fileDestDirs = {
        L"C:\\root\\Images",   // -> bucket 0
        L"c:/root/images",     // -> bucket 0 (CI + sep)
        L"C:\\root\\Docs",     // -> bucket 1
        L"C:\\root\\Other",    // -> no bucket (dropped, as brute force does)
    };

    auto got = AssignFilesToBuckets(fileDestDirs, buckets);
    auto ref = BruteForceAssign(fileDestDirs, buckets);
    check(got == ref, "one-pass assignment equals the O(buckets x files) brute force");
    check(got.size() == 3, "one bucket list per bucket");
    check((got[0] == std::vector<int>{0, 1}), "Images bucket gets files 0 and 1");
    check((got[1] == std::vector<int>{2}), "Docs bucket gets file 2");
    check(got[2].empty(), "Audio is a parent-only bucket -> no files");

    // empty inputs are safe
    check(AssignFilesToBuckets({}, {}).empty(), "no buckets -> empty result");
    check(AssignFilesToBuckets(fileDestDirs, {}).empty(), "no buckets -> empty result (files ignored)");

    // ---- thumbcache: dedup + bounded eviction -----------------------------
    using thumbcache::MaxEntries;
    using thumbcache::ShouldEvict;
    using thumbcache::PickEvictIndex;
    using thumbcache::ShouldEnqueue;

    check(MaxEntries() > 0, "the thumbnail cache has a positive bound");
    check(!ShouldEvict(MaxEntries(), MaxEntries()), "at the cap -> no eviction");
    check(ShouldEvict(MaxEntries() + 1, MaxEntries()), "over the cap -> evict");
    check(!ShouldEvict(0, MaxEntries()), "empty cache -> no eviction");

    // dedup: a key already present must NOT enqueue a second request
    check(ShouldEnqueue(/*keyAlreadyPresent*/false), "new key -> enqueue");
    check(!ShouldEnqueue(/*keyAlreadyPresent*/true), "present key -> do NOT re-enqueue (dedup)");

    // eviction picks the OLDEST completed entry; never an in-flight one
    check(PickEvictIndex(std::vector<bool>{false, true, false, true}) == 1,
          "evict the oldest DONE entry (index 1)");
    check(PickEvictIndex(std::vector<bool>{true}) == 0, "single done entry -> index 0");
    check(PickEvictIndex(std::vector<bool>{false, false}) == -1,
          "all in-flight -> nothing evictable (never evict a pending entry)");
    check(PickEvictIndex(std::vector<bool>{}) == -1, "empty -> nothing evictable");

    if (g_fail) { printf("perf: %d check(s) FAILED\n", g_fail); return 1; }
    printf("perf: all checks passed\n");
    return 0;
}
