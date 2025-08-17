/*
 * type test
 *
 * TODO:
 * - implement cmd + delete (word deletion)
 * - implement multi line text / code blocks
 *
*/
#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_TEXT_LEN 1000
#define MAX_WORDS 50

// State
#define STATE_IDLE 0
#define STATE_ACTIVE 1
#define STATE_ERROR 2



// TEXT SAMPLES
const char* sample_texts[] = {
    "The quick brown fox jumps over the lazy dog.",
    "Programming is the art of telling another human what one wants the computer to do.",
    "In the beginning was the Word, and the Word was with God, and the Word was God.",
    "To be or not to be, that is the question.",
    "All that glitters is not gold, but gold always glitters in the right light.",

    // code blocks
    // "int main() {\n    return 0;\n}"
};

typedef struct {
    char text[MAX_TEXT_LEN];
    char typed[MAX_TEXT_LEN];
    int text_len;
    int typed_len;
    int correct_chars;
    int total_chars;
    time_t start_time;
    time_t end_time;
    int wpm;
    float accuracy;
    int completed;
    bool state;
} TypingTest;

// todo refactor
void init_test(TypingTest* test) {
    int text_index = rand() % (sizeof(sample_texts) / sizeof(sample_texts[0]));
    // int text_index = 5; // code block
    strlcpy(test->text, sample_texts[text_index], strlen(sample_texts[text_index]));
    test->text_len = strlen(test->text);
    test->typed_len = 0;
    test->correct_chars = 0;
    test->total_chars = 0;
    test->completed = 0;
    test->wpm = 0;
    test->accuracy = 0.0;
    test->state = STATE_IDLE;

    memset(test->typed, 0, sizeof(test->typed));
}

void draw_interface(TypingTest* test) {
    clear();
    
    // Title
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(1, (COLS - 20) / 2, "> type");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    // Instructions
    // attron(COLOR_PAIR(2));
    // mvprintw(3, 2, "Type the text below. Press ESC to quit, BACKSPACE to correct mistakes.");
    // attroff(COLOR_PAIR(2));
    
    // Text to type
    int start_row = 6;
    int start_col = 2;
    
    // Display the original text with color coding
    move(start_row, start_col);
    for (int i = 0; i < test->text_len; i++) {
        if (i < test->typed_len) {
            // Already typed characters
            if (test->typed[i] == test->text[i]) {
                attron(COLOR_PAIR(3)); // Green for correct
                addch(test->text[i]);
                attroff(COLOR_PAIR(3));
            } else {
                attron(COLOR_PAIR(4)); // Red for incorrect
                addch(test->text[i]);
                attroff(COLOR_PAIR(4));
            }
        } else if (i == test->typed_len) {
            // Current character (cursor position)
            attron(COLOR_PAIR(5) | A_REVERSE); // Highlighted current char
            addch(test->text[i]);
            attroff(COLOR_PAIR(5) | A_REVERSE);
        } else {
            // Untyped characters
            attron(COLOR_PAIR(6)); // Gray for untyped
            addch(test->text[i]);
            attroff(COLOR_PAIR(6));
        }
    }
    
    // Statistics
    int stats_row = start_row + 4;
    attron(COLOR_PAIR(2));
    mvprintw(stats_row, 2, "Progress: %d/%d characters", test->typed_len, test->text_len);
    mvprintw(stats_row + 1, 2, "WPM: %d", test->wpm);
    mvprintw(stats_row + 2, 2, "Accuracy: %.1f%%", test->accuracy);
    attroff(COLOR_PAIR(2));
    
    if (test->completed) {
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(stats_row + 4, 2, "Test completed! Press 'r' to restart or ESC to quit.");
        attroff(COLOR_PAIR(1) | A_BOLD);
    }
    
    refresh();
}

void calculate_stats(TypingTest* test) {
    if (test->typed_len == 0) {
        test->wpm = 0;
        test->accuracy = 0.0;
        return;
    }
    
    // Calculate accuracy
    int correct = 0;
    for (int i = 0; i < test->typed_len && i < test->text_len; i++) {
        if (test->typed[i] == test->text[i]) {
            correct++;
        }
    }
    test->correct_chars = correct;
    test->accuracy = (float)correct / test->typed_len * 100.0;
    
    // Calculate WPM (Words Per Minute)
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, test->start_time);
    if (elapsed_time > 0) {
        // Assuming average word length of 5 characters
        double words = (double)correct / 5.0;
        double minutes = elapsed_time / 60.0;
        test->wpm = (int)(words / minutes);
    }
        draw_interface(test);
}

void handle_input(TypingTest* test, int ch) {
    if (test->completed) return;
    
    if (test->typed_len == 0) {
        // Start timer on first keystroke
        test->start_time = time(NULL);
        test->state = STATE_ACTIVE;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        // Handle backspace
        if (test->typed_len > 0) {
            test->typed_len--;
            test->typed[test->typed_len] = '\0';
        }
    } else if (ch >= 32 && ch <= 126) {
        // Handle printable characters
        if (test->typed_len < test->text_len) {
            test->typed[test->typed_len] = ch;
            test->typed_len++;
            test->typed[test->typed_len] = '\0';
            
            // Check if test is completed
            if (test->typed_len == test->text_len) {
                test->end_time = time(NULL);
                test->completed = 1;
                test->state = STATE_IDLE;
            }
        }
    }
    
    calculate_stats(test);
}

void setup_colors() {
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Title
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Instructions/Stats
        init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Correct characters
        init_pair(4, COLOR_RED, COLOR_BLACK);     // Incorrect characters
        init_pair(5, COLOR_WHITE, COLOR_BLUE);    // Current character
        init_pair(6, COLOR_WHITE, COLOR_BLACK);   // Untyped characters
    }
}

int main() {
    srand(time(NULL));
    
    // init ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0); // hide cursor
    
    setup_colors();
    
    TypingTest *test = malloc(sizeof(test));
    init_test(test);

    draw_interface(test);
    
    int ch;
    while ((ch = getch()) != 27) {
        // restart test
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || 
           (test->state == STATE_IDLE && (ch == 'r' || ch == 'R'))) {
            init_test(test);
        } 
        else {
            handle_input(test, ch);
        }
        draw_interface(test);
    }
    
    // cleanup
    endwin();
    return 0;
}
