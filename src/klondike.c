#include <stdbool.h>
#include <stdio.h>

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
    for (byte i = 0; i < 4; i++) {
        fprint_card(f, top_card(&game->homes[i]));
    }
    fputc('\n', f);

    for (byte line = 0, empty_piles = 0; empty_piles != 7; line++) {
        empty_piles = 0;
        for (byte i = 0; i < 7; i++) {
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

Card TESTING_DECK[52] = {
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

int main(int argc, char **argv) {
    Klondike game = { 0 };

    byte deck_index = 0;
    for (byte i = 0; i < 7; i++) {
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

    add_card(flip_card(pop_card(&game.stock)), &game.waste);
    render_game_state(stdout, &game);

    add_card(pop_card(&game.piles[6]), &game.homes[0]);
    add_card(flip_card(pop_card(&game.piles[6])), &game.piles[6]);
    render_game_state(stdout, &game);

    return 0;
}
