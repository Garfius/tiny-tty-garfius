import sys
import time

ESC = '\033'
CSI = ESC + '['

def espera():
    #time.sleep(1)
    input()

def write(s):
    sys.stdout.write(s)
    sys.stdout.flush()

def demo(title):
    write(CSI + '0m')  # Reset attributes
    write(f"\n{CSI}1m{'='*40}\n{title:^40}\n{'='*40}{CSI}0m\n")
    espera()

def main():
    demo('Screen Clearing')
    write(CSI + '2J' + CSI + 'H')  # Clear screen
    write('Screen cleared and cursor returned to home.\n')

    demo('Colors (Foreground)')
    for color in range(30, 38):
        write(f"{CSI}{color}mColor FG {color}{CSI}0m\t")
    write('\n')

    demo('Colors (Background)')
    for color in range(40, 48):
        write(f"{CSI}{color}mColor BG {color}{CSI}0m\t")
    write('\n' + CSI + '0m')

    demo('Bright Colors')
    for color in range(90, 98):
        write(f"{CSI}{color}mBright FG {color}{CSI}0m\t")
    write('\n')
    for color in range(100, 108):
        write(f"{CSI}{color}mBright BG {color}{CSI}0m\t")
    write('\n' + CSI + '0m')

    demo('Text Attributes')
    write(f"{CSI}1mBold{CSI}0m\n{CSI}4mUnderline{CSI}0m\n{CSI}7mReverse{CSI}0m\n{CSI}9mStrikethrough{CSI}0m\n")
    write(f"{CSI}3mItalic{CSI}0m\n")

    demo('Cursor Movement')
    write("Line1\nLine2\nLine3\n")
    write(CSI + '1A')  # Move cursor up 1 line
    write(" <- Cursor up one line\n")
    write(CSI + '2B')  # Down 2 lines
    write(" <- Cursor down two lines\n")
    write(CSI + '5C')  # Right 5 chars
    write(" <- Cursor right 5\n")
    write(CSI + '3D')  # Left 3 chars
    write(" <- Cursor left 3\n")
    write(CSI + '5;5H')  # Move to row 1, col 20
    write(" [at 5;5]\n")

    demo('Erase Functions')
    write("\n\rErase entire display:")
    time.sleep(1)
    write(f"{CSI}2J")
    write("\rErased to end of last line:")
    write(f"Hello world{CSI}3D{CSI}K\n")
    write("Erase to start of line:\n")
    time.sleep(1)
    write(f"{CSI}KHello world")
    write("Partial line erase:")
    time.sleep(1)
    write(f"12345{CSI}1K\n")
    write(f"Erase display from cursor down:{CSI}2A")
    time.sleep(1)
    write(f"{CSI}J")
    write("\n")

    demo('Insert/Delete Lines/Chars')
    write("Before insert line:\nLineA\nLineB\n" + CSI + '1A' + CSI + 'L')  # Insert line above LineB
    write("Inserted line above.\n")
    write("Delete this line:\n" + CSI + 'M')
    write("Line was deleted.\n")
    write("Insert 3 chars:" + CSI + '3@')
    write("AAA\n")
    write("123456Delete 2 chars:\r" + CSI + '2P')
    write("Deleted.\n")

    demo('Save')
    write("Saving cursor position..." + CSI + 's')
    write("\nNow moving cursor down 2 lines..." + CSI + '2A')
    time.sleep(1)
    write("Restored curs pos" + CSI + 'u\n')
    time.sleep(1)

    demo('Show/Hide Cursor')
    write(CSI + '?25l')
    write("Hide cursor for 2 seconds.")
    time.sleep(2)
    write(CSI + '?25h')
    write(" Cursor restored.\n")

    demo('Reset All')
    write(CSI + '0mAll attributes reset.\n')

    demo('End of Test')
    write('\n' + '='*40 + '\n')
    write('If you see all demos correctly, your terminal implements common ANSI escape codes!\n')
    write('='*40 + '\n')

if __name__ == "__main__":
    main()