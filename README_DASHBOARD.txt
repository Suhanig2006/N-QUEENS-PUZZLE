N-Queens Puzzle Solver Game — C backtracking + HTML dashboard

Keep nqueens_dashboard.c and nqueens_dashboard_template.html together in one folder. The C program calculates solutions and a detailed event trace, writes nqueens_dashboard.html beside those files, then asks macOS to open the generated page in your default browser.

Mac / VS Code:
1. Open the project folder in VS Code (the folder containing both files).
2. Open Terminal > New Terminal. Confirm `pwd` points to that folder.
3. Compile and run:
   clang -std=c99 -Wall -Wextra nqueens_dashboard.c -o nqueens_dashboard
   ./nqueens_dashboard

Windows (MinGW):
   gcc -std=c99 -Wall -Wextra nqueens_dashboard.c -o nqueens_dashboard.exe
   .\nqueens_dashboard.exe

Linux:
   gcc -std=c99 -Wall -Wextra nqueens_dashboard.c -o nqueens_dashboard
   ./nqueens_dashboard

Dashboard features:
- Explore Solutions: totals and up to 10 boards, with queen coordinates and row-to-column lists.
- Play Puzzle: free placement or guided safe moves; attacking queens are highlighted and conflicts are described by coordinate and type.
- Three-stage hints, full solution reveal, and a valid partial-board challenge. The challenge demonstrates how fixed choices restrict possible completions.
- Step-by-step solver: slow playback by default (1.4 seconds per event), play, pause, single-step, reset, and speed controls. Unsafe squares identify the exact candidate and attacking queen(s), classified as column and/or diagonal conflicts. Row conflicts are prevented by the one-queen-per-row search. The solver pauses when a solution is found.
- Learning section: problem explanation, numbered search steps, pseudocode, and links by function name to the C implementation.
- Search statistics: candidates tested, unsafe candidate checks, queen removals/backtracks, solutions reached, and playback elapsed time. Playback elapsed time is animation time excluding pauses; it is not a CPU performance benchmark.

Expected totals: 4x4 = 2, 6x6 = 4, 8x8 = 92. Rotations and reflections count as distinct arrangements.
