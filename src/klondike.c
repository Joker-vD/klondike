#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COUNTOF(arr)    (sizeof(arr) / sizeof(*(arr)))

typedef unsigned char byte;
typedef signed char sbyte;

typedef enum Suit : byte { SPADES, CLUBS, HEARTS, DIAMONDS } Suit;

bool is_black_suit(Suit suit) { return suit <= CLUBS; }
bool is_red_suit(Suit suit) { return suit >= HEARTS; }

typedef enum Rank : byte { ACE = 1, JACK = 11, QUEEN = 12, KING = 13 } Rank;

typedef struct Card { byte d; } Card;

Card make_card(Rank rank, Suit suit) {
    return (Card){ .d = rank + suit * 16 };
}

#define NO_CARD ((Card){ .d = 0 })

bool is_valid_card(Card card) { return card.d != 0; }
Rank get_rank(Card card) { return card.d % 16; }
Suit get_suit(Card card) { return card.d / 16; }
bool is_face_up(Card card) { return (sbyte)card.d >= 0; }

Card flip_card(Card card) { return (Card){.d = -card.d}; }

const char * const SUIT_SYMBOLS[] = {
    [SPADES]    = "\xE2\x99\xA0",
    [CLUBS]     = "\xE2\x99\xA3",
    [HEARTS]    = "\xE2\x99\xA5",
    [DIAMONDS]  = "\xE2\x99\xA6",
};

const char * const CARDBACK = "\xE2\x96\x92";

int fprint_sigil(FILE *f, Card card) {
    Rank rank = get_rank(card);

    switch (rank) {
    case ACE:
        return fprintf(f, "A%s", SUIT_SYMBOLS[get_suit(card)]);
    case JACK: case QUEEN: case KING:
        return fprintf(f, "%c%s", "JQK"[rank - JACK], SUIT_SYMBOLS[get_suit(card)]);
    default:
        return fprintf(f, "%d%s", rank, SUIT_SYMBOLS[get_suit(card)]);
    }
}

int fprint_card(FILE *f, Card card) {
    if (is_valid_card(card)) {
        if (is_face_up(card)) {
            return fprint_sigil(f, card);
        }
        return fprintf(f, "%s%s", CARDBACK, CARDBACK);
    }
    return fprintf(f, "\x20\x20");
}

typedef struct Depot {
    byte len;
    Card cards[52];
} Depot;

bool is_empty_depot(const Depot *depot) {
    return depot->len == 0;
}

void add_card(Card card, Depot *depot) {
    depot->cards[depot->len++] = card;
}

Card top_card(const Depot *depot) {
    if (is_empty_depot(depot)) {
        return NO_CARD;
    }
    return depot->cards[depot->len-1];
}

Card pop_card(Depot *depot) {
    Card result = top_card(depot);
    if (depot->len != 0) {
        depot->cards[--depot->len] = NO_CARD;
    }
    return result;
}

typedef struct Klondike {
    Depot stock, waste;
    Depot homes[4];
    Depot piles[7];
} Klondike;

void render_game_state(FILE *f, const Klondike *game) {
    fprint_card(f, top_card(&game->stock));
    fputc('\x20', f);
    fprint_card(f, top_card(&game->waste));
    fputs("\x20\x20\x20\x20", f);
    for (byte i = 0; i < COUNTOF(game->homes); i++) {
        fprint_card(f, top_card(&game->homes[i]));
    }
    fputc('\n', f);

    for (byte line = 0, empty_piles = 0; empty_piles != COUNTOF(game->piles); line++) {
        empty_piles = 0;
        for (byte i = 0; i < COUNTOF(game->piles); i++) {
            const Depot *piles = &game->piles[i];
            if (line >= piles->len) {
                empty_piles++;
            }
            if (i != 0) { fputc('\x20', f); }
            fprint_card(f, piles->cards[line]);
        }
        putc('\n', f);
    }
}

typedef Card CardOrPlace;

typedef enum Place : byte {
    HOME1 = 16 * 4, HOME2, HOME3, HOME4,
    PILE1, PILE2, PILE3, PILE4, PILE5, PILE6, PILE7,
} Place;

bool is_place(CardOrPlace x) { return x.d >= HOME1; }

typedef struct MoveCmd {
    Card from;
    CardOrPlace to;
} MoveCmd;

typedef enum CmdKind : byte {
    CMD_QUIT, CMD_MOVE, CMD_DEAL,
} CmdKind;

typedef struct Cmd {
    CmdKind kind;
    union {
        MoveCmd move;
    };
} Cmd;

const char *skip_ws(const char *raw) {
    const char *orig_raw = raw;
    while (*raw == '\x20' || *raw == '\t') { raw++; }
    return raw == orig_raw ? NULL : raw;
}

const char *maybe_skip_ws(const char* raw) {
    while (*raw == '\x20' || *raw == '\t') { raw++; }
    return raw;
}

// RANK  ::=  ["2".."9"] | "10" | "J" | "Q" | "K" | "A"
const char *parse_rank(const char *raw, Rank *rank) {
    if (raw[0] >= '2' && raw[0] <= '9') {
        *rank = rank[0] - '0';
        return &raw[1];
    }

    if (raw[0] == '1' && raw[1] == '0') {
        *rank = 10;
        return &raw[2];
    }

    switch (raw[0]) {
    case 'J':
        *rank = 11;
        return &raw[1];
    case 'Q':
        *rank = 12;
        return &raw[1];
    case 'K':
        *rank = 13;
        return &raw[1];
    case 'A':
        *rank = 1;
        return &raw[1];
    default:
        return NULL;
    }
}

const char *parse_suit(const char *raw, Suit *suit) {
    static const char *PLAIN_SUIT_SYMBOLS = "SCHD";
    const char *s = strchr(PLAIN_SUIT_SYMBOLS, raw[0]);
    if (s != NULL) {
        *suit = s - PLAIN_SUIT_SYMBOLS;
        return &raw[1];
    }

    for (Suit s = 0; s < COUNTOF(SUIT_SYMBOLS); s++) {
        if (strncmp(raw, SUIT_SYMBOLS[s], 3) == 0) {
            *suit = s;
            return &raw[3];
        }
    }

    return NULL;
}

// CARD  ::=  RANK SUIT | SUIT RANK
const char *parse_card(const char *raw, Card *card) {
    Rank rank = 0;
    Suit suit = 0;
    const char *old_raw = raw;
    raw = parse_rank(raw, &rank);

    if (raw != NULL) {
        raw = parse_suit(raw, &suit);
        *card = make_card(rank, suit);
        return raw;
    } else {
        raw = parse_suit(old_raw, &suit);
        if (raw == NULL) { return NULL; }
        raw = parse_rank(raw, &rank);
        *card = make_card(rank, suit);
        return raw;
    }
}

// PLACE  ::=  "EMPTY" ["1".."7"]? | "HOME" ["1".."4"]
const char *parse_card_or_place(const char *raw, CardOrPlace *x) {
    if (strncmp(raw, "EMPTY", sizeof("EMPTY")-1) == 0) {
        raw += sizeof("EMPTY")-1;
        if (raw[0] >= '1' && raw[0] <= '7') {
            x->d = PILE1 + (raw[0] - '1');
            return &raw[1];
        }
        x->d = 0;
        return raw;
    }

    if (strncmp(raw, "HOME", sizeof("HOME")-1) == 0) {
        raw += sizeof("HOME")-1;
        if (raw[0] >= '1' && raw[0] <= '4') {
            x->d = HOME1 + (raw[0] - '1');
            return &raw[1];
        }
    }

    return parse_card(raw, x);
}

// MOVE_CMD  ::=  CARD "TO"? (CARD | PLACE)
bool parse_move_cmd(const char *raw, MoveCmd *cmd) {
    raw = maybe_skip_ws(raw);

    raw = parse_card(raw, &cmd->from);
    if (raw == NULL) { return false; }

    raw = skip_ws(raw);
    if ((raw[0] == 'T' || raw[0] == 't') && (raw[1] == 'O' || raw[1] == 'o')) {
        raw = skip_ws(&raw[2]);
    }
    if (raw == NULL) { return false; }

    raw = parse_card_or_place(raw, &cmd->to);
    if (raw == NULL) { return false; }

    raw = maybe_skip_ws(raw);

    return (raw != NULL && raw[0] == 0);
}

// CMD  ::=  MOVE_CMD | "DEAL" | "" | "Q" "UIT"?
bool parse_cmd(const char *raw, Cmd *cmd) {
    if (raw[0] == 0 || strcmp(raw, "DEAL") == 0) {
        cmd->kind = CMD_DEAL;
        return true;
    }

    if (strcmp(raw, "Q") == 0 || strcmp(raw, "QUIT") == 0) {
        cmd->kind = CMD_QUIT;
        return true;
    }

    if (parse_move_cmd(raw, &cmd->move)) {
        cmd->kind = CMD_MOVE;
        return true;
    }

    return false;
}

const Card TESTING_DECK[52] = {
     { 60 },
     { 43 }, { 6 },
     { 38 }, { 20 }, { 3 },
     { 1 },  { 10 }, { 52 }, { 44 },
     { 37 }, { 27 }, { 24 }, { 2 }, { 21 },
     { 58 }, { 13 }, { 33 }, { 7 }, { 42 }, { 57 },
     { 25 }, { 29 }, { 23 }, { 51 }, { 56 }, { 17 }, { 11 },

     { 34 }, { 53 }, { 5 }, { 49 }, { 45 }, { 50 }, { 26 }, { 35 }, { 39 }, { 8 }, { 59 }, { 28 },
     { 19 }, { 61 }, { 55 }, { 22 }, { 54 }, { 40 }, { 4 }, { 18 }, { 9 }, { 36 }, { 12 }, { 41 },
};

const char* const TESTING_COMMANDS =
    "JS TO QD  \n"
    "  AC HOME4\n"
    "DEAL\n"
    "\n"
    "invalid"
    "\n"
    "h4 to c5\n"
    "q\n"
    "quit\n"
;

char COMMAND_LINE_BUFFER[64];

char* read_command_line(void) {
    static const char *cursor = TESTING_COMMANDS;

    if (cursor == NULL) {
        return NULL;
    }

    const char *next_cursor = strchr(cursor, '\n');
    if (next_cursor != NULL) {
        char *dst;
        for (dst = COMMAND_LINE_BUFFER; cursor != next_cursor; cursor++, dst++) {
            char ch = *cursor;
            if (ch >= 'a' && ch <= 'z') { ch -= 'a' - 'A'; }
            *dst = ch;
        }
        *dst = 0;
        cursor++;
        return COMMAND_LINE_BUFFER;
    } else {
        cursor = NULL;
        return NULL;
    }
}

int main(int argc, char **argv) {
    Klondike game = { 0 };

    byte deck_index = 0;
    for (byte i = 0; i < COUNTOF(game.piles); i++) {
        Depot *pile = &game.piles[i];
        for (byte j = 0; j < i; j++) {
            add_card(flip_card(TESTING_DECK[deck_index++]), pile);
        }
        add_card(TESTING_DECK[deck_index++], pile);
    }

    while (deck_index < 52) {
        add_card(flip_card(TESTING_DECK[deck_index++]), &game.stock);
    }

    render_game_state(stdout, &game);
    while (true) {
        const char *raw = read_command_line();

        if (raw == NULL) {
            return 0;
        }

        Cmd cmd;
        if (parse_cmd(raw, &cmd)) {
            fprintf(stderr, ". '%s'\n", raw);
        } else {
            fprintf(stderr, "! '%s'\n", raw);
        }
    }

    return 0;
}
