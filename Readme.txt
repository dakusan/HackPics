HackPics v1.0.1

Description:
	Reverse engineering project to extract pictures from a PlayStation2 game

Updates/Recent Information:
	See http://www.castledragmire.com/Projects/HackPics for up to date information on this project.

Why I made this program:
    First, I believe this program will probably work on many Playstation/PS2 games (with a possibility of minor tailoring), unfortunately, I cannot find out if this is so, as I do not myself own a PS, PS2, or any games besides the .hack series (don’t ask). So hopefully, with this research, and provided source code, others may be able to make a fully functional utility for the PS’s that can extract this kind of content.

The Story:
    One night after finally finishing the .hack games, I decided I wanted backgrounds from the .hack OS desktop for my real desktops. I scoured the internet to no avail, very disappointed that I was unable to find them. I decided to write this program then and there, which unfortunately, I put on my “to do” list and didn’t get to for about 6 months afterwards :-).

    I worked hard on it when I had free time for about 2 days and extracted all the backgrounds that I wanted (there was one background that has problems which I will get to later on. See the source code comments), and I was going to drop the project there, but decided to go ahead and finish it up because other people might find my research useful.

The Functionality:
    I gave the program simple functionality to be able to browse through all the pictures in the .hack discs. Unfortunately, there are lots of little bugs in there because I was unwilling to spend the extra time reverse engineering the rest of the format. My notes on the remaining bugs are as in the main source file.

How to use:
    	Copy data.bin (from your .hackCD/data folder) to the directory this program is in. You can directly link to it on your DVD-rom drive if you edit the 1st uncommented line in the WinMain function (programming stuff).
    	The listbox controls what picture is displayed on the screen or saved with the buttons.
			Space: goes to the next valid picture (not really needed anymore since at least 90% of the files show pictures).
			1-9: Change the “width” of the picture. The program sometimes inaccurately judges the width (as I did not reverse engineer all the variables). This will be seen as 2 or more almost identical pictures side by side. 5 is the base, 1-4 makes it shrink in 50% intervals, 6-9 makes it grow in 50% intervals. (Example: cw1hsw00_4 should be setting 4).
			All other keys control the listbox as normal (pageup, pagedown, up, down, letters, end, home, etc) Listbox maintains focus.
    	All files saved with the buttons are saved in the same folder that the program is executing in (where the data.bin is).
    	Bitmaps are saved in palette format (usually 8bit/256 colors), as they are originally stored in that format.

Jeffrey Riaboy (Dakusan) 10/31/04