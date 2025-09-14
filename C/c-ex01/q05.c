// A. if ((rook_row + rook_col) % 2 == 0)
//       → "The rook is on a white square"
//    else
//       → "The rook is on a black square"

// B. (rook_row == bishop_row) || (rook_col == bishop_col)

// C. Given expression covers only diagonals with slope +1
//    (top-left ↘ bottom-right).
//    It misses diagonals with slope -1
//    (top-right ↙ bottom-left).
//    Correct expression:
//    abs(bishop_row - rook_row) == abs(bishop_col - rook_col)
