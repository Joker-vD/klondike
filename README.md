# TUI Klondike in C

Have you ever wanted to play Klondike solitaire in your terminal? Have you ever wanted to do it ed-style: that is, by typing in your moves one by one at the command prompt, and then see the updated game state printed out below the command you've just typed? Probably no, you haven't, but now you totally can! Also, vi mode is available for people who like to move the visible cursor on the screen.

## Installation

Requires a Linux distribution with `git`, `make` and a C compiler.

    git clone https://github.com/Joker-vD/klondike.git
    cd klondike
    make

The compiled executable is stored at `bin/klondike`. You can execute it directly, or with `make run` command.

## Configuration

The game recognizes the [`NO_COLOR` environment variable](https://no-color.org/). Use the `--use-color` option to force the use of ANSI color escape sequences. The color escape sequences can be customized by changing the `KLONDIKE_COLORS` environment variable which has the following syntax:

    (("S" | "C" | "H" | "D" | "_")  "="  (anything except "|")*  "|")*

The terminating `|` is optional. The strings between the `=` and `|` are pasted as-is between `\e[` and `m` to form the full escape sequences. Each of the `S`, `C`, `H`, `D`, and `_` headers can only be used once. The `_`-headed string specifies the color of the card face, the `S`-, `C`-, `H`-, and `D`-headed strings specify the colors for spades, clubs, hearts, and diamonds, respectively. Omitting a color specification for a suit/card face from the `KLONDIKE_COLORS` environment variable makes the game use the default color for that suit or the card face.

The default color theme is `S=30|C=34|H=31|D=2;33|_=47` — i.e. black spades, blue clubs, red hearts, diamonds of low-intensity yellow-brownish color that looks like the least awful substitute for orange on the terminal emulators tested, and light gray for the card faces; also known as "Balatro High Contrast". The `NO_COLOR` environment variable is equivalent to using `S=|C=|H=|D=|_=` as the value for `KLONDIKE_COLORS`. Some other interesting color themes are

|Color theme|String|
|---|---|
| Balatro High Contrast, dark face | `S=37\|_=40` |
| Traditional, light face | `C=30\|D=31` |
| Traditional, dark face | `C=37\|D=31\|S=37\|_=40` |
| Four-color poker, light face | `D=34\|C=1;32\|` |
| Four-color poker, dark face | `D=34\|C=1;32\|S=37\|_=40` |
| Interstate 60 | `S=31\|H=30` |

The `--deck` option takes a string representation of an ordered deck to use instead of a random shuffle (e.g. `--deck AS10DKH...`).

The visual size of the cards can be changed by supplying either `--small` (5x1 cards), `--normal` (5x4 cards), or `--large` (7x6 cards) option. By default, the `--normal` option is used.

By default, the game runs in ed mode; to run in vi mode, use the `--vi-mode` option. In vi mode, the standard environment variables `LINES` and `COLUMNS` can be used to override the autodetection of the terminal window's size. Only if both the standard input *and* standard output are connected to a terminal (ideally, the same one) will vi mode be enabled.

## Gameplay (ed mode)

After you enter a valid move, it will be performed and the updated game state will be printed. If you enter an invalid move, it won't be performed but an exclamation mark `!` will be printed instead. If you enter a completely unrecognized command, a question mark `?` will be printed.

The commands are case-insensitive. The syntax of the recognized commands is as following:

    COMMAND  ::=  MOVE_CMD | UP_CMD | DEAL_CMD | QUIT_CMD | RESTART_CMD

    MOVE_CMD     ::=  CARD (" TO")? " " (CARD | PLACE)
    UP_CMD       ::=  CARD (" UP")?
    DEAL_CMD     ::=  "DEAL"?
    QUIT_CMD     ::=  "Q" "UIT"?
    RESTART_CMD  ::=  "R" "ESTART"?

    CARD  ::=  RANK SUIT | SUIT RANK
    RANK  ::=  "A" | ["2".."9"] | "10" | "J" | "Q" | "K"
    SUIT  ::=  "S" | "♠" | "C" | "♣" | "H" | "♥" | "D" | "♦"

    PLACE  ::=  "EMPTY" ["1".."7"]? | "HOME" ["1".."4"]

The "move" command takes a pile built on top of the first card and moves it on top of the second card, or to the named place. The name `EMPTY` specifies the first open spot on the tableau, names `EMPTY1` through `EMPTY7` allow you to target a specific open spot, numbered from left to right, and names `HOME1` through `HOME4` specify home depots, also numbered from left to right.

The "up" command takes the named card and tries to put it into a suitable home depot, if any. It is provided as a convenience, allowing one to type e.g. "AC UP" instead of "AC to home1" and "7H" instead of "7H to 6H".

The "deal" command deals one card from the stock or, if the stock is empty, flips the waste pile into the stock. As is obvious from the syntax description above, it can be entered by simply pressing `[ENTER]` after the prompt.

The "quit" command quits the game. The "restart" command restarts the game with the same shuffling of the deck.

## Gameplay (vi mode)

Move the cursor around the tableau to chose a card/stack of cards to move. Confirm the selection, then move cursor to the place you want to move the selected card/stack and confirm your choice. Repeat until all the cards are at their homes.

The cursor is rendered as `>` and `<` on the border of the card under the cursor; the card itself is rendered in the inverted mode. The selected card is rendered with `&` on its border, and also in the inverted mode.

Movement:

| Key | Description |
|---|---|
| `h` | Move the cursor one depot to the left |
| `j` | Move the cursor one card down in the current depot |
| `k` | Move the cursor one card up in the current depot |
| `l` | Move the cursor one depot to the right |
| `G` | Move the cursor to the bottom of the current depot |

Scrolling:

| Key | Description |
|---|---|
| `^Y` | Scroll the tableau one card up on the screen |
| `^L` | Scroll the tableau one card down on the screen |

Scrolling is only available when the tableau is too large to be fully rendered on the terminal.

Selection:

| Key | Description |
|---|---|
| `SPACE`, `ENTER` | If no card is currently selected, then select the card under cursor — or, if the cursor is at the stock, deal a card. Otherwise, move the selected card/stack of cards to the card/depot under the cursor |
| `x` | Cancel the current selection |

Miscellaneous:

| Key | Description |
|---|---|
| `^L` | Cancel the selection and repaint the screen |
| `:` | Switch to the command mode. In this mode, you can enter a command from ed mode, except that the "deal" command has to be spelt in full: pressing `Enter` with the empty input simply exits the command mode instead of dealing a card |
