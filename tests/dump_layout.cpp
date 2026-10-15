// dump_layout.cpp -- throwaway probe (NOT a *_test.cpp, so the gate ignores it).
// Prints the pinned Group coordinates so pixel-pinned tests can be re-pinned
// after a legitimate geometry change.
#include "layout_logic.h"
#include <cstdio>
using namespace layout;
static void dump(const char* tag, const PromptLayout& g) {
    printf("== %s ==\n", tag);
    printf("windowW=%d windowH=%d\n", g.windowW, g.windowH);
    printf("groupHeader   = {%d,%d,%d,%d}\n", g.groupHeader.x, g.groupHeader.y, g.groupHeader.w, g.groupHeader.h);
    printf("selectedList  = {%d,%d,%d,%d}\n", g.selectedList.x, g.selectedList.y, g.selectedList.w, g.selectedList.h);
    printf("destCard      = {%d,%d,%d,%d}\n", g.destCard.x, g.destCard.y, g.destCard.w, g.destCard.h);
    printf("destLabel.y=%d destDesc.y=%d destField.y=%d\n", g.destLabel.y, g.destDesc.y, g.destField.y);
    printf("nameCard      = {%d,%d,%d,%d}\n", g.nameCard.x, g.nameCard.y, g.nameCard.w, g.nameCard.h);
    printf("nameLabel.y=%d nameDesc.y=%d nameField.y=%d\n", g.nameLabel.y, g.nameDesc.y, g.nameField.y);
    printf("actionCard    = {%d,%d,%d,%d}\n", g.actionCard.x, g.actionCard.y, g.actionCard.w, g.actionCard.h);
    printf("opToggle      = {%d,%d,%d,%d}\n", g.opToggle.x, g.opToggle.y, g.opToggle.w, g.opToggle.h);
    printf("segCopy.y=%d segMove.y=%d\n", g.segCopy.y, g.segMove.y);
    printf("createButton  = {%d,%d} cancelButton = {%d,%d}\n", g.createButton.x, g.createButton.y, g.cancelButton.x, g.cancelButton.y);
}
int main() {
    dump("compact", ComputePromptLayout(true, false));
    dump("expanded", ComputePromptLayout(true, true));
    return 0;
}
