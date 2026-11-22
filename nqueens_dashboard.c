#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_N 8
#define MAX_SOLUTIONS 100
#define MAX_TRACE_EVENTS 100000
#define TRACE_LIMIT 100000

enum { TRY=0, PLACE=1, REJECT=2, FOUND=3, DEAD_END=4, BACKTRACK=5 };
enum { COL_CONFLICT=1, DIAG_CONFLICT=2 };

typedef struct {
    int type, row, col, reason;
    int column_mask, diagonal_mask;
    int positions, unsafe, backtracks, found;
} TraceEvent;

static int n, placement[MAX_N], solution_count;
static int solutions[MAX_SOLUTIONS][MAX_N];
static TraceEvent trace_events[MAX_TRACE_EVENTS];
static int trace_count, positions_tried, unsafe_checks, backtracks, trace_solutions;

static int conflict_reason(int row, int col, int *column_mask, int *diagonal_mask) {
    int reason = 0;
    *column_mask = 0;
    *diagonal_mask = 0;
    for (int r = 0; r < row; ++r) {
        if (placement[r] < 0) continue;
        if (placement[r] == col) { reason |= COL_CONFLICT; *column_mask |= 1 << r; }
        if (abs(placement[r] - col) == row - r) { reason |= DIAG_CONFLICT; *diagonal_mask |= 1 << r; }
    }
    return reason;
}

static void record_event(int type, int row, int col, int reason, int column_mask, int diagonal_mask) {
    if (trace_count >= MAX_TRACE_EVENTS) return;
    TraceEvent *e = &trace_events[trace_count++];
    e->type = type; e->row = row; e->col = col; e->reason = reason;
    e->column_mask = column_mask; e->diagonal_mask = diagonal_mask;
    e->positions = positions_tried; e->unsafe = unsafe_checks;
    e->backtracks = backtracks; e->found = trace_solutions;
}

static void trace_search(int row) {
    if (row == n) {
        ++trace_solutions;
        if (solution_count < MAX_SOLUTIONS)
            memcpy(solutions[solution_count], placement, (size_t)n * sizeof(int));
        ++solution_count;
        record_event(FOUND, row, -1, 0, 0, 0);
        return;
    }
    int had_safe_candidate = 0;
    for (int col = 0; col < n; ++col) {
        ++positions_tried;
        record_event(TRY, row, col, 0, 0, 0);
        int column_mask, diagonal_mask;
        int reason = conflict_reason(row, col, &column_mask, &diagonal_mask);
        if (reason) {
            ++unsafe_checks;
            record_event(REJECT, row, col, reason, column_mask, diagonal_mask);
            continue;
        }
        had_safe_candidate = 1;
        placement[row] = col;
        record_event(PLACE, row, col, 0, 0, 0);
        trace_search(row + 1);
        placement[row] = -1;
        ++backtracks;
        record_event(BACKTRACK, row, col, 0, 0, 0);
    }
    if (!had_safe_candidate) record_event(DEAD_END, row, -1, 0, 0, 0);
}

static void reset_search(int size) {
    n = size; solution_count = 0; trace_count = 0;
    positions_tried = unsafe_checks = backtracks = trace_solutions = 0;
    for (int r = 0; r < MAX_N; ++r) placement[r] = -1;
}

static void emit_trace(FILE *out, int size, int comma) {
    reset_search(size);
    trace_search(0);
    if (trace_count >= MAX_TRACE_EVENTS) {
        fprintf(stderr, "Trace limit reached for N=%d.\n", size);
        exit(1);
    }
    if (comma) fputc(',', out);
    fprintf(out, "{\"n\":%d,\"events\":[", n);
    for (int i = 0; i < trace_count; ++i) {
        TraceEvent *e = &trace_events[i];
        if (i) fputc(',', out);
        fprintf(out, "[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", e->type, e->row, e->col,
                e->reason, e->column_mask, e->diagonal_mask, e->positions,
                e->unsafe, e->backtracks, e->found);
    }
    fprintf(out, "],\"totals\":[%d,%d,%d,%d]}", positions_tried,
            unsafe_checks, backtracks, trace_solutions);
}

static void solution_search(int row) {
    if (row == n) {
        if (solution_count < MAX_SOLUTIONS)
            memcpy(solutions[solution_count], placement, (size_t)n * sizeof(int));
        ++solution_count;
        return;
    }
    for (int col = 0; col < n; ++col) {
        int column_mask, diagonal_mask;
        if (!conflict_reason(row, col, &column_mask, &diagonal_mask)) {
            placement[row] = col;
            solution_search(row + 1);
            placement[row] = -1;
        }
    }
}

static void emit_solutions(FILE *out, int size, int comma) {
    reset_search(size);
    solution_search(0);
    if (comma) fputc(',', out);
    fprintf(out, "{\"n\":%d,\"total\":%d,\"solutions\":[", n, solution_count);
    for (int s = 0; s < solution_count && s < MAX_SOLUTIONS; ++s) {
        if (s) fputc(',', out);
        fputc('[', out);
        for (int row = 0; row < n; ++row) {
            if (row) fputc(',', out);
            fprintf(out, "%d", solutions[s][row]);
        }
        fputc(']', out);
    }
    fputs("]}", out);
}

static int program_folder(const char *program, char *folder, size_t capacity) {
    char resolved[4096];
    if (realpath(program, resolved)) {
        char *slash = strrchr(resolved, '/');
        if (slash) { *slash = '\0'; return snprintf(folder, capacity, "%s", *resolved ? resolved : "/") < (int)capacity; }
    }
    if (strchr(program, '/')) {
        if (snprintf(folder, capacity, "%s", program) >= (int)capacity) return 0;
        char *slash = strrchr(folder, '/');
        if (slash) *slash = '\0';
        return 1;
    }
    return getcwd(folder, capacity) != NULL;
}

int main(int argc, char **argv) {
    (void)argc;
    char folder[4096], template_path[8192], html_path[8192];
    if (!program_folder(argv[0], folder, sizeof folder)) {
        fprintf(stderr, "Could not locate the project folder.\n"); return 1;
    }
    snprintf(template_path, sizeof template_path, "%s/nqueens_dashboard_template.html", folder);
    snprintf(html_path, sizeof html_path, "%s/nqueens_dashboard.html", folder);
    FILE *in = fopen(template_path, "r");
    if (!in) { fprintf(stderr, "Template not found: %s\n", template_path); return 1; }
    FILE *out = fopen(html_path, "w");
    if (!out) { perror(html_path); fclose(in); return 1; }

    char line[4096]; int saw_solutions = 0, saw_trace = 0;
    while (fgets(line, sizeof line, in)) {
        char *marker = strstr(line, "__SOLUTION_DATA__");
        if (marker) {
            fwrite(line, 1, (size_t)(marker - line), out); fputc('[', out);
            emit_solutions(out, 4, 0); emit_solutions(out, 6, 1); emit_solutions(out, 8, 1);
            fputc(']', out); fputs(marker + strlen("__SOLUTION_DATA__"), out); saw_solutions = 1;
            continue;
        }
        marker = strstr(line, "__TRACE_DATA__");
        if (marker) {
            fwrite(line, 1, (size_t)(marker - line), out); fputc('[', out);
            emit_trace(out, 4, 0); emit_trace(out, 6, 1); emit_trace(out, 8, 1);
            fputc(']', out); fputs(marker + strlen("__TRACE_DATA__"), out); saw_trace = 1;
            continue;
        }
        fputs(line, out);
    }
    fclose(in); fclose(out);
    if (!saw_solutions || !saw_trace) {
        fprintf(stderr, "HTML template is missing a data placeholder.\n"); return 1;
    }
    printf("Dashboard generated. Opening it in your browser...\n");
#ifdef __APPLE__
    execlp("open", "open", html_path, (char *)NULL);
#elif defined(_WIN32)
    char command[9000]; snprintf(command, sizeof command, "start \"\" \"%s\"", html_path); return system(command);
#else
    execlp("xdg-open", "xdg-open", html_path, (char *)NULL);
#endif
    fprintf(stderr, "Open this file in a browser: %s\n", html_path);
    return 0;
}
