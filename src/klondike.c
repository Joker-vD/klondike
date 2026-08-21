#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define COUNTOF(arr)    (sizeof(arr) / sizeof(*(arr)))
#define LENOF(str)      (COUNTOF(str) - 1)
#define S(str)          (str), LENOF(str)

#define FOREACH(type, var, arr) for (type var = (arr); var < &(arr)[COUNTOF(arr)]; var++)

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

bool is_same_suit_colors(Card lhs, Card rhs) {
    return is_black_suit(get_suit(lhs)) == is_black_suit(get_suit(rhs));
}

bool equal_cards(Card lhs, Card rhs) { return lhs.d == rhs.d; }

Card hide_card(Card card) { return is_face_up(card) ? (Card){.d = -card.d} : card; }
Card show_card(Card card) { return is_face_up(card) ? card : (Card){.d = -card.d}; }

typedef Card CardOrPlace;

typedef enum Place : byte {
    HOME1 = 16 * 4, HOME2, HOME3, HOME4,
    PILE1, PILE2, PILE3, PILE4, PILE5, PILE6, PILE7,
    STOCK, WASTE,
} Place;

bool is_place(CardOrPlace x) { return x.d >= HOME1; }
Place as_place(CardOrPlace place) { return (Place)place.d; };

typedef struct LineBuffer {
    int fd;
    bool isatty;
    short offset;
    short end;
    char buffer[256];
} LineBuffer;

void lb_flush(LineBuffer *lb) {
    for (short offset = 0, count = lb->offset; count; ) {
        int wrote = write(lb->fd, &lb->buffer[offset], count);

        if (wrote < 0) {
            if (errno == EINTR) {
                wrote = 0;
                continue;
            }

            break;
        }

        offset += wrote;
        count -= wrote;
    }

    lb->offset = 0;
}

void lb_abort(LineBuffer *lb) {
    lb->offset = 0;
}

void lb_putc(LineBuffer *lb, char ch) {
    lb->buffer[lb->offset++] = ch;
    if (lb->offset == sizeof(lb->buffer) || ch == '\n') { lb_flush(lb); }
}

void lb_puts(LineBuffer *lb, const char *str, size_t str_len) {
    for (size_t i = 0; i < str_len; i++) {
        lb_putc(lb, str[i]);
    }
}

const char * const SUIT_SYMBOLS[] = {
    [SPADES]    = "\xE2\x99\xA0",
    [CLUBS]     = "\xE2\x99\xA3",
    [HEARTS]    = "\xE2\x99\xA5",
    [DIAMONDS]  = "\xE2\x99\xA6",
};

const char CARDBACK[]   = "\xE2\x96\x92";
const char VLINE[]      = "\xE2\x94\x82";

void left_padding(LineBuffer *lb, int field_width, int padding) {
    while (padding --> field_width) {
        lb_putc(lb, '\x20');
    }
}

void right_padding(LineBuffer *lb, int field_width, int padding) {
    while (padding ++< -field_width) {
        lb_putc(lb, '\x20');
    }
}

void render_sigil(LineBuffer *lb, Card card, int padding) {
    Rank rank = get_rank(card);
    const char *suit_symbol = SUIT_SYMBOLS[get_suit(card)];
    static const char *RANK_SYMBOLS = "_A23456789*JQK";

    if (rank == 10) {
        left_padding(lb, 3, padding);
        lb_putc(lb, '1');
        lb_putc(lb, '0');
        lb_puts(lb, suit_symbol, strlen(suit_symbol));
        right_padding(lb, 3, padding);
    } else {
        left_padding(lb, 2, padding);
        lb_putc(lb, RANK_SYMBOLS[rank]);
        lb_puts(lb, suit_symbol, strlen(suit_symbol));
        right_padding(lb, 2, padding);
    }
}

void render_card(LineBuffer *lb, Card card) {
    static const char * const SUIT_COLORS[] = {
        [SPADES]    = "30",
        [CLUBS]     = "34",
        [HEARTS]    = "31",
        [DIAMONDS]  = "2;33",
    };

    if (is_valid_card(card)) {
        if (is_face_up(card)) {
            const char *suit_color = SUIT_COLORS[get_suit(card)];
            lb_puts(lb, S(VLINE));
            lb_puts(lb, S("\x1B["));
            lb_puts(lb, suit_color, strlen(suit_color));
            lb_puts(lb, S(";47m"));
            render_sigil(lb, card, -3);
            lb_puts(lb, S("\x1B[m"));
            lb_puts(lb, S(VLINE));
        } else {
            lb_puts(lb, S(VLINE));
            lb_puts(lb, S(CARDBACK));
            lb_puts(lb, S(CARDBACK));
            lb_puts(lb, S(CARDBACK));
            lb_puts(lb, S(VLINE));
        }
    } else {
        lb_puts(lb, S(VLINE));
        lb_puts(lb, S("___"));
        lb_puts(lb, S(VLINE));
    }
}

typedef struct Depot {
    Place place;
    byte len;
    Card cards[52];
} Depot;

void init_depot(Depot *depot, Place place) {
    depot->place = place;
    depot->len = 0;
    FOREACH (Card*, card, depot->cards) {
        *card = NO_CARD;
    }
}

bool is_empty_depot(const Depot *depot) {
    return depot->len == 0;
}

bool is_home(const Depot *depot) {
    return depot->place >= HOME1 && depot->place <= HOME4;
}

bool is_pile(const Depot *depot) {
    return depot->place >= PILE1 && depot->place <= PILE7;
}

sbyte card_index(Card card, const Depot *depot) {
    sbyte i = depot->len;

    while (i --> 0 && !equal_cards(depot->cards[i], card));

    return i;
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

bool is_continuous_run(Card card, const Depot *depot) {
    bool is_legal_move_to_pile(Card from, Card to);

    sbyte index = card_index(card, depot);
    if (index < 0) { return false; }

    for (; index + 1 < depot->len; index++) {
        if (!is_legal_move_to_pile(depot->cards[index + 1], depot->cards[index])) {
            return false;
        }
    }

    return true;
}

typedef enum CardVisibilityAction : byte {
    JUST_MOVE_CARD, HIDE_CARD, SHOW_CARD,
} CardVisibilityAction;

void move_top_card(Depot *from, Depot *to, CardVisibilityAction card_action) {
    Card card = pop_card(from);

    if (is_valid_card(card)) {
        switch (card_action) {
        case JUST_MOVE_CARD:
            break;
        case HIDE_CARD:
            card = hide_card(card);
            break;
        case SHOW_CARD:
            card = show_card(card);
            break;
        }

        add_card(card, to);
    }
}

void move_run(Depot *from, Card start_card, Depot *to) {
    sbyte start_index = card_index(start_card, from);
    if (start_index < 0) { return; }

    for (byte i = start_index; i < from->len; i++) {
        add_card(from->cards[i], to);
        from->cards[i] = NO_CARD;
    }

    from->len = start_index;
}

typedef struct Klondike {
    Depot stock, waste;
    Depot homes[4];
    Depot piles[7];
} Klondike;

bool is_game_won(const Klondike *game) {
    FOREACH (const Depot*, home, game->homes) {
        if (get_rank(top_card(home)) != KING) {
            return false;
        }
    }

    return true;
}

Depot *place_to_depot(Klondike *game, Place place) {
    if (place >= HOME1 && place <= HOME4) {
        return &game->homes[place - HOME1];
    }
    if (place >= PILE1 && place <= PILE7) {
        return &game->piles[place - PILE1];
    }

    if (place == STOCK) { return &game->stock; }
    if (place == WASTE) { return &game->waste; }

    return NULL;
}

Depot *find_depot_with_card(Klondike *game, Card card) {
    if (!is_valid_card(card)) { return NULL; }

    FOREACH (Depot *, pile, game->piles) {
        if (card_index(card, pile) != -1) { return pile; }
    }

    FOREACH (Depot *, home, game->homes) {
        if (card_index(card, home) != -1) { return home; }
    }

    if (card_index(card, &game->waste) != -1) { return &game->waste; }
    if (card_index(card, &game->stock) != -1) { return &game->stock; }

    return NULL;
}

Depot *find_empty_home(Klondike *game) {
    FOREACH (Depot *, home, game->homes) {
        if (is_empty_depot(home)) { return home; }
    }

    return NULL;
}

Depot *find_empty_pile(Klondike *game) {
    FOREACH (Depot *, pile, game->piles) {
        if (is_empty_depot(pile)) { return pile; }
    }

    return NULL;
}

bool is_legal_move_to_pile(Card from, Card to) {
    if (!is_face_up(from)) {
        return false;
    }

    if (!is_valid_card(to)) {
        return get_rank(from) == KING;
    }

    if (!is_face_up(to)) {
        return false;
    }

    return get_rank(from) + 1 == get_rank(to) && !is_same_suit_colors(from, to);
}

bool is_legal_move_to_home(Card from, Card to) {
    if (!is_face_up(from)) {
        return false;
    }

    if (!is_valid_card(to)) {
        return get_rank(from) == ACE;
    }

    if (!is_face_up(to)) {
        return false;
    }

    return get_rank(from) == get_rank(to) + 1 && get_suit(from) == get_suit(to);
}

bool try_deal_card(Klondike *game) {
    if (is_empty_depot(&game->stock)) {
        if (is_empty_depot(&game->waste)) {
            return false;
        }

        while (!is_empty_depot(&game->waste)) {
            move_top_card(&game->waste, &game->stock, HIDE_CARD);
        }
        return true;
    }

    move_top_card(&game->stock, &game->waste, SHOW_CARD);
    return true;
}

bool try_move_card(Klondike *game, Card from_card, CardOrPlace to) {
    Depot *from_depot = find_depot_with_card(game, from_card);
    if (from_depot == NULL || from_depot->place == STOCK) {
        return false;
    }

    Depot *to_depot;
    Card to_card = NO_CARD;

    if (is_place(to)) {
        to_depot = place_to_depot(game, as_place(to));
        if (to_depot == NULL || to_depot->place == STOCK || to_depot->place == WASTE) {
            return false;
        }
        to_card = top_card(to_depot);

    } else if (is_valid_card(to)) {
        to_depot = find_depot_with_card(game, to);
        if (to_depot == NULL || to_depot->place == STOCK || to_depot->place == WASTE) {
            return false;
        }
        to_card = to;

    } else {
        switch (get_rank(from_card)) {
        case ACE:
            to_depot = find_empty_home(game);
            break;
        case KING:
            to_depot = find_empty_pile(game);
            break;
        default:
            to_depot = NULL;
            break;
        }

        if (to_depot == NULL) {
            return false;
        }
    }

    if (from_depot == to_depot || !equal_cards(top_card(to_depot), to_card)) {
        return false;
    }

    if (is_pile(to_depot) && (
            !is_legal_move_to_pile(from_card, to_card) ||
            !is_continuous_run(from_card, from_depot)))
    {
        return false;
    }


    if (is_home(to_depot) && (
            !is_legal_move_to_home(from_card, to_card) ||
            !equal_cards(top_card(from_depot), from_card)))
    {
        return false;
    }


    move_run(from_depot, from_card, to_depot);
    move_top_card(from_depot, from_depot, SHOW_CARD);

    return true;
}

bool try_up_card(Klondike *game, Card card) {
    Depot *from_depot = find_depot_with_card(game, card);
    if (from_depot == NULL || from_depot->place == STOCK ||
        (from_depot->place >= HOME1 && from_depot->place <= HOME4) ||
        !equal_cards(top_card(from_depot), card))
    {
        return false;
    }

    Depot *to_home;
    switch (get_rank(card)) {
    case ACE:
        to_home = find_empty_home(game);
        break;
    default:
        to_home = find_depot_with_card(game, make_card(get_rank(card) - 1, get_suit(card)));
        if (to_home != NULL && (to_home->place < HOME1 || to_home->place > HOME4)) {
            to_home = NULL;
        }
        break;
    }

    if (to_home == NULL) {
        return false;
    }

    if (!is_legal_move_to_home(card, top_card(to_home))) {
        return false;
    }

    move_top_card(from_depot, to_home, JUST_MOVE_CARD);
    move_top_card(from_depot, from_depot, SHOW_CARD);
    return true;
}

void render_game_state(LineBuffer *output, const Klondike *game) {
    render_card(output, top_card(&game->stock));
    lb_putc(output, '\x20');
    render_card(output, top_card(&game->waste));
    lb_puts(output, S("\x20" "\x20\x20\x20\x20\x20" "\x20"));
    FOREACH (const Depot *, home, game->homes) {
        if (home != game->homes) { lb_putc(output, '\x20'); }
        render_card(output, top_card(home));
    }
    lb_putc(output, '\n');
    lb_putc(output, '\n');

    for (byte line = 0, empty_piles = 0; ; line++) {
        empty_piles = 0;
        FOREACH (const Depot *, pile, game->piles) {
            if (pile != game->piles) { lb_putc(output, '\x20'); }

            if (line >= pile->len) {
                if (line == 0) {
                    render_card(output, NO_CARD);
                } else {
                    lb_puts(output, S("\x20\x20\x20\x20\x20"));
                    empty_piles++;
                }
            } else {
                render_card(output, pile->cards[line]);
            }
        }

        if (empty_piles != COUNTOF(game->piles)) {
            lb_putc(output, '\n');
        } else {
            lb_abort(output);
            break;
        }
    }
}

typedef struct MoveCmd {
    Card from;
    CardOrPlace to;
} MoveCmd;

typedef struct UpCmd {
    Card card;
} UpCmd;

typedef enum CmdKind : byte {
    CMD_QUIT, CMD_MOVE, CMD_UP, CMD_DEAL,
} CmdKind;

typedef struct Cmd {
    CmdKind kind;
    union {
        MoveCmd move;
        UpCmd up;
    };
} Cmd;

const char *maybe_skip_ws(const char *raw) {
    while (*raw == '\x20' || *raw == '\t') { raw++; }
    return raw;
}

const char *skip_ws(const char *raw) {
    const char *orig_raw = raw;
    raw = maybe_skip_ws(raw);
    return raw == orig_raw ? NULL : raw;
}

const char *maybe_skip_str(const char *raw, const char *str, size_t str_len) {
    if (strncmp(raw, str, str_len) == 0) {
        return raw + str_len;
    }
    return raw;
}

const char *skip_str(const char *raw, const char *str, size_t str_len) {
    const char *orig_raw = raw;
    raw = maybe_skip_str(raw, str, str_len);
    return raw == orig_raw ? NULL : raw;
}

// RANK  ::=  ["2".."9"] | "10" | "J" | "Q" | "K" | "A"
const char *parse_rank(const char *raw, Rank *rank) {
    if (raw[0] >= '2' && raw[0] <= '9') {
        *rank = raw[0] - '0';
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
    const char *s = memchr(PLAIN_SUIT_SYMBOLS, raw[0], strlen(PLAIN_SUIT_SYMBOLS));
    if (s != NULL) {
        *suit = s - PLAIN_SUIT_SYMBOLS;
        return &raw[1];
    }

    for (Suit s = 0; s < COUNTOF(SUIT_SYMBOLS); s++) {
        if (strncmp(raw, SUIT_SYMBOLS[s], strlen(SUIT_SYMBOLS[s])) == 0) {
            *suit = s;
            return &raw[strlen(SUIT_SYMBOLS[s])];
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
    const char *orig_raw = raw;

    raw = maybe_skip_str(raw, S("EMPTY"));
    if (raw != orig_raw) {
        if (raw[0] >= '1' && raw[0] <= '7') {
            x->d = PILE1 + (raw[0] - '1');
            return &raw[1];
        }
        x->d = 0;
        return raw;
    }

    raw = maybe_skip_str(raw, S("HOME"));
    if (raw != orig_raw) {
        if (raw[0] >= '1' && raw[0] <= '4') {
            x->d = HOME1 + (raw[0] - '1');
            return &raw[1];
        }
    }

    return parse_card(raw, x);
}

// MOVE_CMD  ::= CARD WS ("TO" WS)? (CARD | PLACE)
const char *parse_move_cmd(const char *raw, MoveCmd *cmd) {
    raw = parse_card(raw, &cmd->from);
    if (raw == NULL) { return NULL; }

    raw = skip_ws(raw);
    if (raw == NULL) { return NULL; }

    if (raw[0] == 'T' && raw[1] == 'O') {
        raw = skip_ws(&raw[2]);
    }
    if (raw == NULL) { return NULL; }

    raw = parse_card_or_place(raw, &cmd->to);

    return raw;
}

// UP_CMD  ::=  CARD (WS "UP")?
const char *parse_up_cmd(const char *raw, UpCmd *cmd) {
    raw = parse_card(raw, &cmd->card);
    if (raw == NULL) { return NULL; }

    if (raw[0] == '\x20' || raw[1] == '\t') {
        raw = skip_ws(raw);

        if (raw[0] == 'U' && raw[1] == 'P') {
            raw = &raw[2];
            return raw;
        }
        return NULL;
    }

    return raw;
}

// DEAL_CMD  ::=  "DEAL"?
const char* parse_deal_cmd(const char *raw) {
    if (raw[0] == 0) { return raw; }

    raw = skip_str(raw, S("DEAL"));
    return raw;
}

// QUIT_CMD  ::= "Q" "UIT"?
const char *parse_quit_cmd(const char *raw) {
    if (raw[0] != 'Q') { return false; }
    raw++;

    raw = maybe_skip_str(raw, S("UIT"));
    return raw;
}

bool parse_ws_eol(const char *raw) {
    raw = maybe_skip_ws(raw);
    return raw[0] == 0;
}

// CMD  ::=  WS? (MOVE_CMD | UP_CMD | DEAL_CMD | QUIT_CMD) WS?
bool parse_cmd(const char *raw, Cmd *cmd) {
    const char *orig_raw = maybe_skip_ws(raw);

    raw = parse_deal_cmd(orig_raw);
    if (raw != NULL && parse_ws_eol(raw)) {
        cmd->kind = CMD_DEAL;
        return true;
    }

    raw = parse_quit_cmd(orig_raw);
    if (raw != NULL && parse_ws_eol(raw)) {
        cmd->kind = CMD_QUIT;
        return true;
    }

    raw = parse_move_cmd(orig_raw, &cmd->move);
    if (raw != NULL && parse_ws_eol(raw)) {
        cmd->kind = CMD_MOVE;
        return true;
    }

    raw = parse_up_cmd(orig_raw, &cmd->up);
    if (raw != NULL && parse_ws_eol(raw)) {
        cmd->kind = CMD_UP;
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

bool lb_fill_rdbuf(LineBuffer *lb) {
    lb->offset = lb->end = 0;

    while (true) {
        int count = read(lb->fd, lb->buffer, sizeof(lb->buffer));

        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }

        lb->end = count;
        return true;
    }
}

// Reads bytes from the input and writes them into the buffer, until either buffer_size bytes are written,
// or LF is written, or EOF/error is encountered. Returns the number of bytes written or, if 0 would be
// returned because of encountering EOF/error early, returns -1 instead.
int lb_gets(LineBuffer *lb, char *buffer, int buffer_size) {
    if (buffer_size < 0) { return -1; }
    if (buffer_size == 0) { return 0; }

    int total_read_count = 0;

    do {
        if (lb->offset == lb->end) {
            if (!lb_fill_rdbuf(lb)) {
                if (total_read_count == 0) { total_read_count = -1; }
                break;
            }
        }

        int len = lb->end - lb->offset;
        if (len > buffer_size - total_read_count) { len = buffer_size - total_read_count; }

        char *newline = memchr(&lb->buffer[lb->offset], '\n', len);
        if (newline != NULL) {
            newline++;
            len = newline - &lb->buffer[lb->offset];
            memcpy(buffer, &lb->buffer[lb->offset], len);
            lb->offset += len;
            total_read_count += len;
            break;
        } else {
            memcpy(buffer, &lb->buffer[lb->offset], len);
            buffer += len;
            lb->offset += len;
            total_read_count += len;
        }
    } while(total_read_count < buffer_size);

    return total_read_count;
}

// After reading a line that fits into the buffer, returns the number of bytes read.
// After reading an overly long line, skips it and returns 0. On read error, returns -1.
int get_short_line(LineBuffer *input, char *buffer, int buffer_size) {
    if (buffer_size < 0) { return -1; }

    // Any line is overly long for a zero-sized buffer, so skip it, more or less efficiently.
    if (buffer_size == 0) {
        static char tmp[64];
        if (get_short_line(input, tmp, sizeof(tmp)) < 0) {
            return -1;
        }
        return 0;
    }

    int read_count = lb_gets(input, buffer, buffer_size);
    if (read_count < 0) {
        return -1;
    }

    if (buffer[read_count - 1] == '\n') {
        return read_count;
    }

    // Skip overly long line
    do {
        read_count = lb_gets(input, buffer, buffer_size);
        if (read_count < 0) {
            return -1;
        }
    } while (buffer[read_count - 1] != '\n');

    return 0;
}

char *read_command_line(LineBuffer *input, LineBuffer *output) {
    static char COMMAND_LINE_BUFFER[64];

    do {
        if (input->isatty && output->isatty) {
            lb_puts(output, S("> "));
            lb_flush(output);
        }

        int read_count = get_short_line(input, COMMAND_LINE_BUFFER, sizeof(COMMAND_LINE_BUFFER));
        if (read_count < 0) {
            if (input->isatty && output->isatty) {
                lb_putc(output, '\n');
            }
            return NULL;
        }

        if (read_count == 0) {
            continue;
        }

        COMMAND_LINE_BUFFER[read_count - 1] = 0;
        for (char *s = COMMAND_LINE_BUFFER; s < &COMMAND_LINE_BUFFER[read_count]; s++) {
            if (*s >= 'a' && *s <= 'z') { *s -= 'a' - 'A'; }
        }

        return COMMAND_LINE_BUFFER;
    } while(true);
}

void start_game(Klondike *game, const Card deck[static 52]) {
    init_depot(&game->stock, STOCK);
    init_depot(&game->waste, WASTE);

    FOREACH (Depot *, home, game->homes) {
        init_depot(home, HOME1 + (home - game->homes));
    }

    byte deck_index = 0;
    FOREACH (Depot *, pile, game->piles) {
        byte i = pile - game->piles;
        init_depot(pile, PILE1 + i);
        for (byte j = 0; j < i; j++) {
            add_card(hide_card(deck[deck_index++]), pile);
        }
        add_card(show_card(deck[deck_index++]), pile);
    }

    while (deck_index < 52) {
        add_card(hide_card(deck[deck_index++]), &game->stock);
    }
}

typedef enum GameResult : byte {
    GAME_QUIT, GAME_WON,
} GameResult;

GameResult play_game(LineBuffer *input, LineBuffer *output, Klondike *game, const Card shuffled_deck[static 52]) {
    start_game(game, TESTING_DECK);

    while (true) {
        render_game_state(output, game);

        if (is_game_won(game)) {
            return GAME_WON;
        }

        for (bool command_succeeded = false; !command_succeeded; ) {
            const char *raw = read_command_line(input, output);

            if (raw == NULL) {
                return GAME_QUIT;
            }

            Cmd cmd;
            if (!parse_cmd(raw, &cmd)) {
                lb_puts(output, S("?\n"));
                continue;
            }

            switch (cmd.kind) {
            case CMD_QUIT:
                return GAME_QUIT;
            case CMD_DEAL:
                command_succeeded = try_deal_card(game);
                break;
            case CMD_MOVE:
                command_succeeded = try_move_card(game, cmd.move.from, cmd.move.to);
                break;
            case CMD_UP:
                command_succeeded = try_up_card(game, cmd.up.card);
                break;
            }

            if (!command_succeeded) {
                lb_puts(output, S("!\n"));
            }
        }
    }
}

int main(int argc, char **argv) {
    Klondike game = { 0 };
    LineBuffer stdin  = { .fd = 0, .isatty = isatty(0), 0 };
    LineBuffer stdout = { .fd = 1, .isatty = isatty(1), 0 };

    if (play_game(&stdin, &stdout, &game, TESTING_DECK) == GAME_WON) {
        if (stdout.isatty) {
            lb_putc(&stdout, '\a');
        }
        lb_puts(&stdout, S("You won!\n"));
    }

    return 0;
}
