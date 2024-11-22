#ifndef TYPES_H
#define TYPES_H

enum RefreshType {
    NO_REFRESH,
    SOFT_REFRESH, // Partial refresh - faster but may leave artifacts
    FULL_REFRESH  // Complete refresh - slower but cleaner display
};

#endif // TYPES_H