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

bool is_valid_card(Card card) { return card.d != 0; }
Rank get_rank(Card card) { return card.d % 16; }
Suit get_suit(Card card) { return card.d / 16; }
bool is_face_up(Card card) { return (sbyte)card.d >= 0; }

const char * const SUIT_SYMBOLS[] = {
    [SPADES]    = "\xE2\x99\xA0",
    [CLUBS]     = "\xE2\x99\xA3",
    [HEARTS]    = "\xE2\x99\xA5",
    [DIAMONDS]  = "\xE2\x99\xA6",
};

int fprint_sigil(FILE *f, Card card) {
    if (is_valid_card(card)) {
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
    return 0;
}

typedef struct Depot { byte len; Card cards[52]; } Depot;

bool is_empty_depot(const Depot *depot) { return depot->len == 0; }
void add_card(Card card, Depot* depot) { depot->cards[depot->len++] = card; }

int main(int argc, char **argv) {
    Depot depots[4] = { 0 };

    for (Suit suit = SPADES; suit <= DIAMONDS; suit++) {
        for (Rank rank = ACE; rank <= KING; rank++) {
            add_card(make_card(rank, suit), &depots[suit]);
        }
    }

    for (const Depot *depot = &depots[0]; depot < &depots[4]; depot++) {
        for (byte i = 0; i < depot->len; i++) {
            if (i != 0) { fputc('\x20', stdout); }
            fprint_sigil(stdout, depot->cards[i]);
        }
        fputc('\n', stdout);
    }
    return 0;
}
