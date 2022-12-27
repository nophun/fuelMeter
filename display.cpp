/*   display.c - Number display driver   */
/*   Yuster, Liangyu Chen   */

#include <arduino.h>
#include <string.h>
#include <inttypes.h>
#include "display.h"

static uint8_t cursor_col;
static uint8_t cursor_row;
static uint8_t num_rows;
static uint8_t num_cols;

const struct font0507 ns0507[] = {n0507_0, n0507_1, n0507_2, n0507_3, n0507_4, n0507_5, n0507_6, n0507_7, n0507_8, n0507_9};
const struct font0507 cs0507_up[] = {c0507_A, c0507_B, c0507_C, c0507_D, c0507_E, c0507_F, c0507_G, c0507_H, c0507_I, c0507_J, c0507_K, c0507_L, c0507_M, c0507_N, c0507_O, c0507_P, c0507_Q, c0507_R, c0507_S, c0507_T, c0507_U, c0507_V, c0507_W, c0507_X, c0507_Y, c0507_Z, c0507_UNKN};
const struct font0507 cs0507_low[] = {c0507_a, c0507_b, c0507_c, c0507_d, c0507_e, c0507_f, c0507_g, c0507_h, c0507_i, c0507_j, c0507_k, c0507_l, c0507_m, c0507_n, c0507_o, c0507_p, c0507_q, c0507_r, c0507_s, c0507_t, c0507_u, c0507_v, c0507_w, c0507_x, c0507_y, c0507_z, c0507_UNKN};
const struct font0507 s0507[] = {c0507_UNKN, s0507_1, s0507_2, s0507_3, s0507_4, s0507_5, s0507_6, s0507_7, s0507_8, s0507_9, s0507_10, s0507_11, s0507_12, s0507_13, s0507_14, s0507_15};
const struct font1014 ns1014[] = {n1014_0, n1014_1, n1014_2, n1014_3, n1014_4, n1014_5, n1014_6, n1014_7, n1014_8, n1014_9};
const struct font1014 s1014[] = {c1014_UNKN, s1014_1, s1014_2, s1014_3, s1014_4};
const struct font1521 ns1521[] = {n1521_0, n1521_1, n1521_2, n1521_3, n1521_4, n1521_5, n1521_6, n1521_7, n1521_8, n1521_9};
const struct font1521 cs1521_up[] = {c1521_E, c1521_N, c1521_O, c1521_S, c1521_Y, c1521_UNKN};
const struct font1521 s1521[] = {c1521_UNKN, s1521_1, s1521_2, s1521_3, s1521_4, s1521_5};
const struct font2028 ns2028[] = {n2028_0, n2028_1, n2028_2, n2028_3, n2028_4, n2028_5, n2028_6, n2028_7, n2028_8, n2028_9};
const struct font2028 cs2028_up[] = {c2028_E, c2028_N, c2028_O, c2028_S, c2028_Y, c2028_UNKN};
const struct font2028 s2028[] = {c2028_UNKN, s2028_1, s2028_2, s2028_3, s2028_4};

struct unit_struct unit_details[LAST_UNIT + 1] = {
    {"-",    "kPa", 0},
    {"-.--", "bar", 2},
    {"-",    "#C ", 0},
    {"-",    "#F ", 0},
    {"-",    "k/h", 0},
    {"-",    "mph", 0},
    {"-",    "%  ", 0},
    {"-",    "rpm", 0},
    {"-.--", "V  ", 2},
    {"-",    "g/s", 0},
    {"-",    "Nm ", 0},
    {"-",    "L/h", 0},
    {"-",    "#  ", 0}, // degree
    {"-",    "s  ", 0},
    {"-",    "min", 0},
    {"-",    "l  ", 0},
    {"-",    "l/L", 0},
    {"-",    "   ", 0}, // none
    {"-",    "   ", 0},
};

struct unit_struct *get_units(void) {
    return unit_details;
}

void init_display(uint8_t cols, uint8_t rows) {
    num_cols = cols;
    num_rows = rows;
    cursor_col = 0;
    cursor_row = 0;
}

/* Set cursor */
int set_cursor(uint8_t row, uint8_t col) {
    if(row < num_rows) {
        cursor_row = row;
    } else{
        cursor_row = num_rows-1;
    }

    if(col < num_cols) {
        cursor_col = col;
    } else{
        cursor_col = num_cols-1;
    }

    return cursor_row == row && cursor_col == col;
}

/* Set row */
int set_row(uint8_t row) {
    if(row < num_rows) {
        cursor_row = row;
    } else {
        cursor_row = num_rows-1;
    }

    return cursor_row == row;
}

/* Set column */
int set_col(uint8_t col) {
    if(col < num_cols) {
        cursor_col = col;
    } else {
        cursor_col = num_cols-1;
    }

    return cursor_col == col;
}

/* Write a character to data buffer */
int put_font0507(uint32_t *data, struct font0507 ch) {
    uint32_t mask;
    uint32_t word;
    uint8_t shift;

    if(cursor_row >= num_rows) {
        return 0;
    }

    for (int i = 0; i < 5+1; ++i) {
        if(cursor_col >= num_cols) {
            return 0;
        }

        shift = cursor_col%4 * 8;
        mask = 0xff << shift;
        word = data[cursor_col/4 + cursor_row*(num_cols/4)];

        // Empty column after a character
        if(i == 5) {
            data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask);
        } else {
            data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask) | (ch.col[i] << (shift));
        }

        cursor_col++;
    }
    
    return 1;
}

/* Write a character to data buffer in double size */
int put_font1014(uint32_t *data, struct font1014 ch) {
    uint32_t mask;
    uint32_t word;
    uint8_t shift;

    uint8_t orig_col = cursor_col;
    uint8_t orig_row = cursor_row;
    for (int h = 0; h < 2; ++h) {
        if(cursor_row >= num_rows) {
            cursor_row = orig_row;
            return 0;
        }

        cursor_col = orig_col;
        
        for (int i = 0; i < 10+2; ++i) {
            if(cursor_col >= num_cols) {
                break;
            }

            shift = cursor_col%4 * 8;
            mask = 0xff << shift;
            word = data[cursor_col/4 + cursor_row*(num_cols/4)];

            if(i >= 10) {
                data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask);
            } else {
                data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask) | (((ch.col[i] >> (h*8)) & 0x000000ff) << (shift));
            }
            cursor_col++;
        }
        cursor_row++;
    }
    cursor_row = orig_row;

    return 1;
}

/* Write a character to data buffer in triple size */
int put_font1521(uint32_t *data, struct font1521 ch) {
    uint32_t mask;
    uint32_t word;
    uint8_t shift;

    uint8_t orig_col = cursor_col;
    uint8_t orig_row = cursor_row;
    for (int h = 0; h < 3; ++h) {
        if(cursor_row >= num_rows) {
            cursor_row = orig_row;
            return 0;
        }

        cursor_col = orig_col;
        
        for (int i = 0; i < 15+3; ++i) {
            if(cursor_col >= num_cols) {
                break;
            }

            shift = cursor_col%4 * 8;
            mask = 0xff << shift;
            word = data[cursor_col/4 + cursor_row*(num_cols/4)];

            if(i >= 15) {
                data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask);
            } else {
                data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask) | (((ch.col[i] >> (h*8)) & 0x000000ff) << (shift));
            }
            cursor_col++;
        }
        cursor_row++;
    }
    cursor_row = orig_row;

    return 1;
}

/* Write a character to data buffer in quadruple size */
int put_font2028(uint32_t *data, struct font2028 ch) {
    uint32_t mask;
    uint32_t word;
    uint8_t shift;

    uint8_t orig_col = cursor_col;
    uint8_t orig_row = cursor_row;
    for (int h = 0; h < 4; ++h) {
        if(cursor_row >= num_rows) {
            cursor_row = orig_row;
            return 0;
        }

        cursor_col = orig_col;
        
        for (int i = 0; i < 20+4; ++i) {
            if(cursor_col >= num_cols) {
                break;
            }

            shift = cursor_col%4 * 8;
            mask = 0xff << shift;
            word = data[cursor_col/4 + cursor_row*(num_cols/4)];

            if(i >= 20) {
                data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask);
            } else {
                data[cursor_col/4 + cursor_row*(num_cols/4)] = (word & ~mask) | (((ch.col[i] >> (h*8)) & 0x000000ff) << (shift));
            }
            cursor_col++;
        }
        cursor_row++;
    }
    cursor_row = orig_row;

    return 1;
}

/* Print text */
void print_font0507(uint32_t *data, const char *text) {
    while(*text != '\0') {
        switch(*text) {
            case ' ':
                put_font0507(data, s0507[1]);
                break;
            case '.':
                put_font0507(data, s0507[2]);
                break;
            case ',':
                put_font0507(data, s0507[3]);
                break;
            case ':':
                put_font0507(data, s0507[4]);
                break;
            case ';':
                put_font0507(data, s0507[5]);
                break;
            case '-':
                put_font0507(data, s0507[6]);
                break;
            case '+':
                put_font0507(data, s0507[7]);
                break;
            case '_':
                put_font0507(data, s0507[8]);
                break;
            case '%':
                put_font0507(data, s0507[11]);
                break;
            case '/':
                put_font0507(data, s0507[12]);
                break;
            case '#':
                put_font0507(data, s0507[10]);
                break;
            case '?':
                put_font0507(data, s0507[13]);
                break;
            case '>':
                put_font0507(data, s0507[14]);
                break;
            case '<':
                put_font0507(data, s0507[15]);
                break;
            default:
                if(*text >= '0' && *text <= '9') {
                    put_font0507(data, ns0507[*text-'0']);
                } else if(*text >= 'A' && *text <= 'Z') {
                    put_font0507(data, C0507(*text));
                } else if(*text >= 'a' && *text <= 'z') {
                    put_font0507(data, C0507(*text));
                } else {
                    put_font0507(data, s0507[0]);
                }
                break;
        }
        text++;
    }
}

/* Print text in double size */
void print_font1014(uint32_t *data, const char *text) {
    while(*text != '\0') {
        switch(*text) {
            case ' ':
                put_font1014(data, s1014[1]);
                break;
            case '.':
                put_font1014(data, s1014[2]);
                break;
            case ',':
                put_font1014(data, s1014[3]);
                break;
            case '-':
                put_font1014(data, s1014[4]);
                break;
            default:
                if(*text >= '0' && *text <= '9') {
                    put_font1014(data, ns1014[*text-'0']);
                } else if(*text >= 'A' && *text <= 'Z') {
                    put_font1014(data, s1014[0]);
                } else if(*text >= 'a' && *text <= 'z') {
                    put_font1014(data, s1014[0]);
                } else {
                    put_font1014(data, s1014[0]);
                }
                break;
        }
        text++;
    }
}

/* Print text in triple size */
void print_font1521(uint32_t *data, const char *text) {
    while(*text != '\0') {
        switch(*text) {
            case ' ':
                put_font1521(data, s1521[1]);
                break;
            case '.':
                put_font1521(data, s1521[2]);
                break;
            case ',':
                put_font1521(data, s1521[3]);
                break;
            case ':':
                put_font1521(data, s1521[4]);
                break;
            case '-':
                put_font1521(data, s1521[5]);
                break;
            case 'E':
                put_font1521(data, cs1521_up[0]);
                break;
            case 'N':
                put_font1521(data, cs1521_up[1]);
                break;
            case 'O':
                put_font1521(data, cs1521_up[2]);
                break;
            case 'S':
                put_font1521(data, cs1521_up[3]);
                break;
            case 'Y':
                put_font1521(data, cs1521_up[4]);
                break;
            default:
                if(*text >= '0' && *text <= '9') {
                    put_font1521(data, ns1521[*text-'0']);
                } else if(*text >= 'A' && *text <= 'Z') {
                    put_font1521(data, s1521[0]);
                } else if(*text >= 'a' && *text <= 'z') {
                    put_font1521(data, s1521[0]);
                } else {
                    put_font1521(data, s1521[0]);
                }
                break;
        }
        text++;
    }
}

/* Print text in quadruple size */
void print_font2028(uint32_t *data, const char *text) {
    while(*text != '\0') {
        switch(*text) {
            case ' ':
                put_font2028(data, s2028[1]);
                break;
            case '.':
                put_font2028(data, s2028[2]);
                break;
            case ',':
                put_font2028(data, s2028[3]);
                break;
            case '-':
                put_font2028(data, s2028[4]);
                break;
            case 'E':
                put_font2028(data, cs2028_up[0]);
                break;
            case 'N':
                put_font2028(data, cs2028_up[1]);
                break;
            case 'O':
                put_font2028(data, cs2028_up[2]);
                break;
            case 'S':
                put_font2028(data, cs2028_up[3]);
                break;
            case 'Y':
                put_font2028(data, cs2028_up[4]);
                break;
            default:
                if(*text >= '0' && *text <= '9') {
                    put_font2028(data, ns2028[*text-'0']);
                } else if(*text >= 'A' && *text <= 'Z') {
                    put_font2028(data, s2028[0]);
                } else if(*text >= 'a' && *text <= 'z') {
                    put_font2028(data, s2028[0]);
                } else {
                    put_font2028(data, s2028[0]);
                }
                break;
        }
        text++;
    }
}

/* Print header */
void print_header(uint32_t *data, const char *text, Alignment alignment) {
    uint8_t len = strlen(text);
    uint8_t left_fill = 0;
    uint8_t right_fill = 0;

    if(len > c0507_MAXLEN) {
        return;
    }

    set_cursor(0, 0);

    if (alignment == Alignment::Right) {
        right_fill = c0507_MAXLEN-len;
    } else if (alignment == Alignment::Center) {
        right_fill = (c0507_MAXLEN-len)/2;
    }

    // Put left fillers
    for (int i = 0; i < right_fill; ++i) {
        put_font0507(data, s0507[1]);
    }
    print_font0507(data, text);

    if (alignment == Alignment::Left) {
        left_fill = c0507_MAXLEN-len;
    } else if (alignment == Alignment::Center) {
        left_fill = (c0507_MAXLEN-len+1)/2;
    }

    // Put right fillers
    for (int i = 0; i < left_fill; ++i) {
        put_font0507(data, s0507[1]);
    }
}

/* Value in double size */
void print_value(uint32_t *data, const char *text) {
    // cprintf("Value: %s\n", text);

#ifdef SINGLE
    uint8_t len = strlen(text);

    if(len > 18) {
        return;
    }

    set_cursor(VALUE_START_ROW, 0);

    // Put fillers
    for (int i = 0; i < 18-len; ++i) {
        put_font0507(data, s0507[1]);
    }

    print_font0507(data, text);
#endif
#ifdef DOUBLE
    uint8_t len = strlen(text);

    if(len > 9) {
        return;
    }

    set_cursor(VALUE_START_ROW, 0);

    // Put fillers
    for (int i = 0; i < 9-len; ++i) {
        put_font1014(data, s1014[1]);
    }

    print_font1014(data, text);
#endif
#ifdef TRIPLE
    uint8_t len = strlen(text);

    if(len > 6) {
        return;
    }

    set_cursor(VALUE_START_ROW, 2);

    // Put fillers
    for (int i = 0; i < 6-len; ++i) {
        put_font1521(data, s1521[1]);
    }

    print_font1521(data, text);
#endif
#ifdef QUADRO
    uint8_t len = strlen(text);

    if(len > 4) {
        return;
    }

    set_cursor(VALUE_START_ROW, 14);

    // Put fillers
    for (int i = 0; i < 4-len; ++i) {
        put_font2028(data, s2028[1]);
    }

    print_font2028(data, text);
#endif
}

/* Print unit */
void print_unit(uint32_t *data, enum UNITS unit) {
    struct unit_struct *unit_list = get_units();

#ifdef SINGLE
    set_cursor(0 + VALUE_START_ROW, UNIT_POSITION);
#endif
#ifdef DOUBLE
    set_cursor(1 + VALUE_START_ROW, UNIT_POSITION);
#endif
#ifdef TRIPLE
    set_cursor(2 + VALUE_START_ROW, UNIT_POSITION);
#endif
#ifdef QUADRO
    set_cursor(3 + VALUE_START_ROW, UNIT_POSITION);
#endif

    if(unit < LAST_UNIT) {
        print_font0507(data, unit_list[unit].unit_txt);
        // cprintf("Unit: %s\n", unit_list[unit].unit_txt);
    } else {
        print_font0507(data, unit_list[LAST_UNIT].unit_txt);
    }
}

void debug_data(uint32_t *data) {
    uint32_t b, w, mask;
    for (int y = 0; y < num_rows; ++y) {
        for (int i = 0; i < 8; ++i) {
            for (int x = 0; x < num_cols; ++x) {
                b = x%4;
                w = x/4;
                mask = ((0x01 << i%8) << 8*b);
 
                Serial.print((data[y * (num_cols/4) + w] & mask)?"x":" ");
            }
            Serial.println();
        }
    }
}