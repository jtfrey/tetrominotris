# tetrominotris

Because I've shown in interest in videos about programming and emulation, YouTube loves to suggest similar content to me.  One night as I was doing the dishes it chose to show me a video by OneLoneCoder wherein he tackled that wonderful game called Tetris.  As he was discussing how he would represent the 4x4 tetromino pieces as 16-character strings my reaction was to wonder why not just use a 16-bit integer, with each bit being an un/occupied state.  Translation within the 4x4 cell could easily be handled as bit shift operations, as shown in Figure 1:

<img src="./assets/TetrominoReps.png" width="480" align="center"/>

The game allows the tetromino piece to be rotated 90, 180, and 270 degrees as it falls, so there are nominally 4 representations for each piece (in some cases symmetry reduces it to 2 or 1 unique representation).  But 4 x 16-bit integers is just a single 64-bit integer, so all orientations of a tetromino will fit in a 64-bit integer (call it a 64-bit word), as shown for the "T" tetromino above.  Each orientation can be extracted by means of a bitwise AND and a bit shift:

```
orientation_4 = (tetromino & 0x000000000000FFFF)
orientation_3 = (tetromino & 0x00000000FFFF0000) >> 16
orientation_2 = (tetromino & 0x0000FFFF00000000) >> 32
orientation_1 = (tetromino & 0xFFFF000000000000) >> 48
```

An important part of the game is collision detection.  When the player attempts to move the piece left or right on the game board, the game must determine if that move is obstructed by pieces already placed on the board.  Likewise, when the piece drops a row — either due to soft or hard drop or the game's periodic gravity — if a collision is detected the action does not yield a move, rather the final placement of the tetromino on the game board.

Extracting a 4x4 region of the game board state will also yield a 16-bit word representing the un/occupied state of the enclosed cells.  If the in-play tetromino is at (4,5) on the game board, a move to the right would put it at (5,5).  If the 4x4 region of state at (5,5) is extracted, collision detection is quite simple:

<img src="./assets/TetrominoCollision.png" width="480" align="center"/>

When the bitwise AND yields zero, there is no collision, and non-zero implies a collision would occur.  Thus, the move to the right is valid while the move down will place the tetromino on the board at its current position, (4,5).  That act of placing the in-play tetromino on the board happens with a bitwise OR rather than an AND, with the result being written back to the board data:

<img src="./assets/TetrominoPlacement.png" width="480" align="center"/>

Rats, I really should have moved to the right that time.

The standard Tetris game board is 10 columns by 20 rows, so it could be represented as an array of 20 16-bit words.  There would be 6 unused bits in each word, with the usable bits masked by the value `03FF` and the unused portion by `FC00`.  Checking for a filled row is a very simple operation:

```
(row & 0x03FF) == 0x03FF
```

Finding completed rows is a straightforward (and quick) bitwise AND and comparison (subtraction).  Removing a row amounts to moving all words preceding it over top of it and filling-in at the head of the array with zeroes.

The heart of tetrominotris is a bit grid C pseudo-class that handles all of the bit ops discussed.

## Color

As origially written the game was black and white only.  Color requires that each position on the game board not only have the un/occupied bit, but some number of additional bits representing a color.  The traditional NES variant of Tetris used a single 4-color palette for the game pieces in each level, so going the same direction we need just 2 additional bits for color data.  The bit grid was augmented to allow for 1 to 8 distinct bit planes.  Row removal shifts data in all planes, and the rest of the operations allow for selection of one or more target bit planes.

For black and white display, the game uses a single bit plane and the un/occupied state maps to black/white.  In color mode, the un/occupied state determines the fill style and bit planes 1 and 2 form a two-bit color index (`0b00 = 0`, `0b01 = 1`, `0b10 = 2`, `0b11 = 3`).
