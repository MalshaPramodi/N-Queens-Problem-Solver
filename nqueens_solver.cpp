#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <atomic>

using namespace std;

// Structure to hold solutions and counts for each worker thread
struct ThreadData
{
    vector<vector<int>> solutions; // Stores found solutions
    uint64_t count;                // Stores count of solutions
};
// Bitwise N-Queens Solver with Multi-threading and Optimized I/O
// Global for limited enumeration
const int MAX_SOLUTIONS_TO_PRINT = 1000;
atomic<int> globalSolutionsCollected(0);
mutex solutionMutex;
vector<vector<int>> limitedSolutions;

/**
 * Bitwise backtracking solver - COUNT MODE
 * Ultra-fast counting without storing solutions
 *
 * @param row Current row being processed
 * @param n Board size
 * @param cols Bitmask of occupied columns
 * @param ld Bitmask of occupied left diagonals
 * @param rd Bitmask of occupied right diagonals
 * @param mask Valid position mask (all 1s up to n bits)
 * @param count Pointer to solution counter
 */

// Bitwise solver - COUNT MODE
inline void solveCount(int row, int n, uint64_t cols, uint64_t ld, uint64_t rd,
                       uint64_t mask, uint64_t *count)
{
    // Base case: all queens placed successfully
    if (row == n)
    {
        (*count)++;
        return;
    }

    uint64_t available = mask & ~(cols | ld | rd);

    // Try each available position
    while (available)
    {
        // Extract rightmost set bit (lowest available column)
        uint64_t bit = available & -available;
        available ^= bit;

        // Recursively solve for next row with updated attack patterns
        solveCount(row + 1, n, cols | bit, (ld | bit) << 1, (rd | bit) >> 1,
                   mask, count);
    }
}

/**
 * Limited enumeration solver - stops after collecting MAX_SOLUTIONS_TO_PRINT
 * Used for large N where full enumeration is impractical
 *
 * @param row Current row
 * @param n Board size
 * @param cols Column occupation bitmask
 * @param ld Left diagonal occupation bitmask
 * @param rd Right diagonal occupation bitmask
 * @param board Array storing queen positions (column index for each row)
 * @param mask Valid position mask
 */
inline void solveLimited(int row, int n, uint64_t cols, uint64_t ld, uint64_t rd,
                         int *board, uint64_t mask)
{
    if (globalSolutionsCollected >= MAX_SOLUTIONS_TO_PRINT)
        return;

    // Base case: solution found
    if (row == n)
    {
        lock_guard<mutex> lock(solutionMutex);
        if (globalSolutionsCollected < MAX_SOLUTIONS_TO_PRINT)
        {
            limitedSolutions.emplace_back(board, board + n);
            globalSolutionsCollected++;
        }
        return;
    }

    uint64_t available = mask & ~(cols | ld | rd);

    while (available && globalSolutionsCollected < MAX_SOLUTIONS_TO_PRINT)
    {
        uint64_t bit = available & -available;
        available ^= bit;

        board[row] = __builtin_ctzll(bit);
        solveLimited(row + 1, n, cols | bit, (ld | bit) << 1, (rd | bit) >> 1,
                     board, mask);
    }
}

/**
 * Full enumeration solver - finds ALL solutions
 * Used for smaller N where complete enumeration is feasible
 *
 * @param row Current row
 * @param n Board size
 * @param cols Column bitmask
 * @param ld Left diagonal bitmask
 * @param rd Right diagonal bitmask
 * @param board Queen positions array
 * @param solutions Vector to store all solutions
 * @param mask Valid position mask
 */
inline void solveFull(int row, int n, uint64_t cols, uint64_t ld, uint64_t rd,
                      int *board, vector<vector<int>> &solutions, uint64_t mask)
{
    if (row == n)
    {
        solutions.emplace_back(board, board + n);
        return;
    }

    uint64_t available = mask & ~(cols | ld | rd);

    while (available)
    {
        uint64_t bit = available & -available;
        available ^= bit;

        board[row] = __builtin_ctzll(bit);
        solveFull(row + 1, n, cols | bit, (ld | bit) << 1, (rd | bit) >> 1,
                  board, solutions, mask);
    }
}

/**
 * Worker thread function for counting mode
 * Explores subtree starting with queen in specific column
 */
void workerCount(int col, int n, uint64_t mask, ThreadData *data)
{
    uint64_t bit = 1ULL << col;
    data->count = 0;
    solveCount(1, n, bit, bit << 1, bit >> 1, mask, &(data->count));
}

/**
 * Worker thread function for full enumeration
 * Finds all solutions starting with queen in specific column
 */
void workerFull(int col, int n, uint64_t mask, ThreadData *data)
{
    int board[32];
    uint64_t bit = 1ULL << col;
    board[0] = col;

    data->solutions.reserve(150000);
    solveFull(1, n, bit, bit << 1, bit >> 1, board, data->solutions, mask);
}

/**
 * Worker thread function for limited enumeration
 * Finds up to MAX_SOLUTIONS_TO_PRINT starting with specific column
 */
void workerLimited(int col, int n, uint64_t mask)
{
    int board[32];
    uint64_t bit = 1ULL << col;
    board[0] = col;
    solveLimited(1, n, bit, bit << 1, bit >> 1, board, mask);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = inputFile;
    size_t pos = outputFile.rfind(".txt");
    if (pos != string::npos)
    {
        outputFile.replace(pos, 4, "_output.txt");
    }
    else
    {
        outputFile += "_output.txt";
    }

    ifstream fin(inputFile);
    if (!fin)
    {
        cerr << "Error: Cannot open input file" << endl;
        return 1;
    }

    auto startTime = chrono::high_resolution_clock::now();

    int N;
    fin >> N;
    fin.close();

    // Handle edge cases
    if (N == 2 || N == 3)
    {
        ofstream fout(outputFile);
        fout << "No Solution" << endl;
        fout.close();

        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = endTime - startTime;
        cout << "N=" << N << " | Time: " << elapsed.count() << "s" << endl;
        return 0;
    }

    if (N == 1)
    {
        ofstream fout(outputFile);
        fout << "1\n1\n1\n";
        fout.close();

        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = endTime - startTime;
        cout << "N=1 | Solutions: 1 | Time: " << elapsed.count() << "s" << endl;
        return 0;
    }

    // Initialize bitmask for valid positions
    uint64_t mask = (1ULL << N) - 1;

    // Exploit horizontal symmetry: only solve for half the first row positions
    int half = N / 2;

    vector<ThreadData> threadData(half);
    vector<thread> threads;
    threads.reserve(half);

    // Mode selection:
    // - N < 18: Full enumeration (fast enough)
    // - N >= 18: Count all + enumerate first 1000 solutions
    bool limitedMode = (N >= 18);

    if (limitedMode)
    {
        // ==================== LIMITED MODE (N >= 18) ====================
        // Step 1: Count all solutions parallel processing
        for (int col = 0; col < half; col++)
        {
            threads.emplace_back(workerCount, col, N, mask, &threadData[col]);
        }

        for (auto &t : threads)
        {
            t.join();
        }

        // Aggregate counts: multiply by 2 for symmetry
        uint64_t totalCount = 0;
        for (int i = 0; i < half; i++)
        {
            totalCount += threadData[i].count * 2;
        }

        // Handle middle column if N is odd (no symmetry pair)
        if (N % 2 == 1)
        {
            ThreadData middleData;
            workerCount(half, N, mask, &middleData);
            totalCount += middleData.count;
        }

        // Step 2: Collect first 1000 solutions (single-threaded, stops early)
        limitedSolutions.reserve(MAX_SOLUTIONS_TO_PRINT);
        globalSolutionsCollected = 0;

        for (int col = 0; col < N && globalSolutionsCollected < MAX_SOLUTIONS_TO_PRINT; col++)
        {
            workerLimited(col, N, mask);
        }

        // Step 3: Write output (count + limited solutions)
        FILE *file = fopen(outputFile.c_str(), "w");
        if (!file)
        {
            cerr << "Error: Cannot write output file" << endl;
            return 1;
        }

        setvbuf(file, nullptr, _IOFBF, 262144); // 256KB buffer
        fprintf(file, "%d\n%llu\n", N, totalCount);

        char line[256];
        for (const auto &sol : limitedSolutions)
        {
            int len = 0;
            for (int j = 0; j < N; j++)
            {
                len += sprintf(line + len, "%d", sol[j] + 1);
                if (j < N - 1)
                    line[len++] = ' ';
            }
            line[len++] = '\n';
            fwrite(line, 1, len, file);
        }

        fclose(file);

        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = endTime - startTime;

        cout << "N=" << N << " | Total Solutions: " << totalCount
             << " | Printed: " << limitedSolutions.size()
             << " | Time: " << elapsed.count() << "s" << endl;
    }
    else
    {
        // ==================== FULL MODE (N < 18) ====================
        // Launch parallel workers for first half of columns
        for (int col = 0; col < half; col++)
        {
            threads.emplace_back(workerFull, col, N, mask, &threadData[col]);
        }

        for (auto &t : threads)
        {
            t.join();
        }

        // Calculate total solutions using symmetry
        size_t totalEstimate = 0;
        for (int i = 0; i < half; i++)
        {
            totalEstimate += threadData[i].solutions.size() * 2;
        }

        // Handle middle column for odd N
        ThreadData middleData;
        if (N % 2 == 1)
        {
            int board[32];
            uint64_t bit = 1ULL << half;
            board[0] = half;
            middleData.solutions.reserve(50000);
            solveFull(1, N, bit, bit << 1, bit >> 1, board, middleData.solutions, mask);
            totalEstimate += middleData.solutions.size();
        }

        // Ultra-fast C-style file writing
        FILE *file = fopen(outputFile.c_str(), "w");
        if (!file)
        {
            cerr << "Error: Cannot write output file" << endl;
            return 1;
        }

        setvbuf(file, nullptr, _IOFBF, 1048576); // 1MB buffer
        fprintf(file, "%d\n%zu\n", N, totalEstimate);

        char line[256];

        // Write original and mirrored solutions
        for (int i = 0; i < half; i++)
        {
            for (const auto &sol : threadData[i].solutions)
            {
                // Original solution
                int len = 0;
                for (int j = 0; j < N; j++)
                {
                    len += sprintf(line + len, "%d", sol[j] + 1);
                    if (j < N - 1)
                        line[len++] = ' ';
                }
                line[len++] = '\n';
                fwrite(line, 1, len, file);

                // Mirrored solution (N - col gives horizontal mirror)
                len = 0;
                for (int j = 0; j < N; j++)
                {
                    len += sprintf(line + len, "%d", N - sol[j]);
                    if (j < N - 1)
                        line[len++] = ' ';
                }
                line[len++] = '\n';
                fwrite(line, 1, len, file);
            }
        }

        // Write middle column solutions
        if (N % 2 == 1)
        {
            for (const auto &sol : middleData.solutions)
            {
                int len = 0;
                for (int j = 0; j < N; j++)
                {
                    len += sprintf(line + len, "%d", sol[j] + 1);
                    if (j < N - 1)
                        line[len++] = ' ';
                }
                line[len++] = '\n';
                fwrite(line, 1, len, file);
            }
        }

        fclose(file);

        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = endTime - startTime;

        cout << "N=" << N << " | Solutions: " << totalEstimate
             << " | Time: " << elapsed.count() << "s" << endl;
    }

    return 0;
}