#ifndef TYPES_H
#define TYPES_H

enum RefreshType {
    NO_REFRESH,               // No refresh (default)
    REFETCH_ELEMENTS,         // Refresh the elements
    ELEMENT_REFRESH_FAST,     // Fast refresh - Flash background color on area before drawing
    ELEMENT_REFRESH_PARTIAL,  // Partial refresh - 2 cycle black to white flash on area before drawing
    ELEMENT_REFRESH_COMPLETE, // Complete refresh - 4 cycles black to white flash on area before drawing
    DISPLAY_REFRESH_FAST,     // Fast refresh - Flash background color on area before drawing
    DISPLAY_REFRESH_PARTIAL,  // Partial refresh - 2 cycle black to white flash on entire display
    DISPLAY_REFRESH_COMPLETE  // Complete refresh - 4 cycles black to white flash on entire display
};

#endif // TYPES_H