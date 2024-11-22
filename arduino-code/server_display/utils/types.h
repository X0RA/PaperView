#ifndef TYPES_H
#define TYPES_H

enum RefreshType {
    NO_REFRESH,   // No refresh
    FAST_REFRESH, // Fast refresh - 1 cycle refresh
    SOFT_REFRESH, // Partial refresh - 2 cycles refresh
    FULL_REFRESH  // Complete refresh - 4 cycles refresh
};

#endif // TYPES_H