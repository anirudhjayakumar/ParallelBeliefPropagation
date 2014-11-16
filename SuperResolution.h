#pragma once

#include <vector>
#include <cstdlib>
#include <cstring>
#include <cassert>

typedef int PatchID;

typedef struct {
    int i, j;
    PatchID id;
} patch_t;
