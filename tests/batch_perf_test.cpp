// batch_perf_test.cpp
// Guards the COST of MakeUniqueFolderNames, not just its output.
//
// The original implementation restarted numbering from "(2)" for every slot in
// the batch, so filling N slots in a directory that already holds M "New folder"
// entries cost O(N * (M + N)) existence probes -- for the 100-folder hard cap in
// a busy directory that is thousands of disk stats on the UI worker thread.
//
// The optimized generator advances a monotonic index cursor and never rescans,
// so the number of existence probes is LINEAR in (count + preexisting). This
// test fails against the quadratic implementation and passes once it is linear.
//
// Output correctness is covered by multi_name_test.cpp; here we only measure the
// probe count while confirming the produced names are still correct.
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <set>
#include <string>
#include <cstdio>

static int g_fail = 0;

int main() {
    const std::wstring dir = L"C:\\x";

    // Directory already holds "New folder" and "New folder (2)"..(50).
    const int preexisting = 50;
    std::set<std::wstring> existing = { L"C:\\x\\New folder" };
    for (int i = 2; i <= preexisting; ++i) {
        wchar_t buf[64] = {0};
        std::swprintf(buf, 64, L"C:\\x\\New folder (%d)", i);
        existing.insert(buf);
    }

    long probes = 0;
    auto exists = [&](const std::wstring& p) -> bool {
        ++probes;                       // one existence probe == one disk stat
        return existing.count(p) > 0;
    };

    const int count = 100;              // the hard cap
    auto got = MakeUniqueFolderNames(dir, count, exists);

    // Correctness: names continue after the pre-existing block, no gaps/dupes.
    if ((int)got.size() != count) {
        printf("[FAIL] expected %d names, got %zu\n", count, got.size());
        ++g_fail;
    }
    std::set<std::wstring> uniq(got.begin(), got.end());
    if (uniq.size() != got.size()) {
        printf("[FAIL] batch produced duplicate names\n");
        ++g_fail;
    }
    if (!got.empty() && got.front() != L"C:\\x\\New folder (51)") {
        printf("[FAIL] first new name must be 'New folder (51)', got %ls\n",
               got.front().c_str());
        ++g_fail;
    }

    // Cost: probing must be LINEAR in (count + preexisting), not quadratic.
    // A generous linear ceiling (still far below the quadratic ~N*(M+N)).
    const long linearCeiling = 3L * (count + preexisting);
    if (probes > linearCeiling) {
        printf("[FAIL] too many existence probes: %ld (linear ceiling %ld). "
               "Numbering is being restarted per slot (quadratic).\n",
               probes, linearCeiling);
        ++g_fail;
    } else {
        printf("  probes=%ld (linear ceiling %ld)\n", probes, linearCeiling);
    }

    if (g_fail == 0) {
        printf("batch_perf_test: PASS\n");
        return 0;
    }
    printf("batch_perf_test: FAIL (%d)\n", g_fail);
    return 1;
}
