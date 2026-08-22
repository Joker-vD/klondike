# TUI Klondike in C

Have you ever wanted to play Klondike solitaire in your terminal? Have you ever wanted to do it ed-style: that is, by typing in your moves one by one at the command prompt, and then see the updated game state printed out below the command you've just typed? Probably no, you haven't, but now you totally can!

## Installation

Requires a Linux distribution with `git`, `make` and a C compiler.

    git clone https://github.com/Joker-vD/klondike.git
    cd klondike
    make

The compiled executable is stored at `bin/klondike`. You can execute it directly, or with `make run` command.

## Configuration

The game recognizes the [`NO_COLOR` environment variable](https://no-color.org/). Use the `--use-color` option to force the use of ANSI color escape sequences.

The `--deck` option takes a string representation of an ordered deck to use instead of a random shuffle (e.g. `--deck AS10DKH...`).

## Gameplay

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
