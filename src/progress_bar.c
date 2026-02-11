#include "progress_bar.h"
#include <stdio.h>

// ANSI color codes
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"

void progress_bar(int current, int total) {
    int bar_width = 40;
    float progress = (float)current / total;
    int filled = (int)(progress * bar_width);

    // Choose color based on progress
    const char *color;
    if (progress < 0.33) color = YELLOW;
    else if (progress < 0.66) color = CYAN;
    else color = GREEN;

    // Draw progress bar
    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf(GREEN "#" RESET);
        } else {
            printf("-");
        }
    }

    printf("] %s%3d%%%s (%d/%d)", color, (int)(progress * 100), RESET, current, total);

    // Show completion
    if (current >= total) {
        printf(" " GREEN "[DONE]" RESET);
    }

    fflush(stdout);

    // Newline when complete
    if (current >= total) {
        printf("\n");
    }
}
