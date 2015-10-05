//--User variables--
#include <windows.h>
const MyWidth=254, MyHeight=120;
const char TitleString[]="HackPics";
const char VersionString[]="v1.01";
const char ExplainString[]="Reverse engineering project to extract\npictures from a PlayStation2 game";
const char *ProjectLink="http://www.castledragmire.com/Projects/HackPics";
const RECT ExplainRect={0, 36, MyWidth, 36+41};
//--End of user variables--

HWND ParentHwnd=NULL, AboutHwnd=NULL; //Global about and hidden parent window handle
void OpenAboutWindow(HWND Main); //Open about window
LRESULT APIENTRY AboutWinProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam); //Process window messages
void DisplaySignature(HDC MyDC, int x, int y);

void OpenAboutWindow(HWND Main)
{
	if(AboutHwnd) //If about form is already open, do nothing
		return;

	//Create window class
	HINSTANCE hInst=(HINSTANCE)GetModuleHandle(NULL);
	const WNDCLASS wc = {CS_HREDRAW | CS_VREDRAW, AboutWinProc, 0, 0, hInst, LoadIcon(hInst, "MAIN_ICON"), LoadCursor(NULL,IDC_ARROW), GetSysColorBrush(COLOR_BTNFACE), NULL, "AboutWindow"};
	RegisterClass(&wc);

	//Create a message window to hold both windows
	if(!ParentHwnd)
	{
		ParentHwnd=CreateWindow("AboutWindow", "Hidden Parent", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, hInst, NULL); //Create the hidden parent window
		SetWindowLong(ParentHwnd, GWL_WNDPROC, (LONG)DefWindowProc); //Window has no special message hooks
		SetWindowLong(Main, GWL_HWNDPARENT, (LONG)ParentHwnd); //Set hidden parent window of main window

		//Force taskbar icon
		SetWindowLong(Main, GWL_EXSTYLE, GetWindowLong(Main, GWL_EXSTYLE)|WS_EX_APPWINDOW);
		ShowWindow(ParentHwnd, SW_SHOW);
		SetActiveWindow(Main);
		ShowWindow(ParentHwnd, SW_HIDE);
	}

	//Calculate window dimensions
	RECT MyWindow={0, 0, MyWidth, MyHeight};
	AdjustWindowRect(&MyWindow, WS_CAPTION|WS_POPUPWINDOW|WS_VISIBLE, false);
	MyWindow.left=MyWindow.right-MyWindow.left;
	MyWindow.top=MyWindow.bottom-MyWindow.top;

	//Create window
	AboutHwnd=CreateWindow("AboutWindow", "About", WS_CAPTION|WS_POPUPWINDOW|WS_VISIBLE, (GetSystemMetrics(SM_CXSCREEN)-MyWindow.left)/2, (GetSystemMetrics(SM_CYSCREEN)-MyWindow.top)/2, MyWindow.left,MyWindow.top, ParentHwnd, NULL, hInst, NULL);
}

LRESULT APIENTRY AboutWinProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	//Font handles
	static HFONT TitleFont, VersionFont, ExplainFont, LinkFont; 
	//static int HorPix; //Currently not used

	//Link edit box hook
	typedef LRESULT (APIENTRY *TypeWinProc)(HWND,UINT,WPARAM,LPARAM); //Windows message listener procedure type
	static TypeWinProc LinkBoxProc; //Original link control handler
	static HWND LinkHwnd=NULL; //Handle to link control
	if(hwnd==LinkHwnd) //Process link edit box messages
	{
		if(msg==WM_LBUTTONDBLCLK) //On double click, open link to project page
			ShellExecute(NULL, "open", ProjectLink, NULL, NULL, SW_SHOWDEFAULT);
		return LinkBoxProc(hwnd,msg,wParam,lParam);
	}

	//Normal about window messages
	switch(msg)
	{
		case WM_ACTIVATE:
		{
			//Get device caps for font height - Should find font sizes by Height*HorPix/72, but since this doesn't seem to work, so I used arbitrary numbers instead to match VB about form layout
			/*HDC MyDC=GetDC(hwnd);
			SetMapMode(MyDC, MM_TEXT);
			HorPix=GetDeviceCaps(MyDC, LOGPIXELSY);
			ReleaseDC(hwnd, MyDC);*/

			//Title Font
			LOGFONT TempFont={37, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DECORATIVE, "Arial"}; //Font Size=24
			TitleFont=CreateFontIndirect(&TempFont);
			//Version Font
			TempFont.lfWeight=FW_DONTCARE;
			TempFont.lfHeight=12; //Font Size=7
			VersionFont=CreateFontIndirect(&TempFont);
			//Explain Font
			TempFont.lfHeight=14; //Font Size=8
			strcpy(TempFont.lfFaceName, "Times New Roman");
			ExplainFont=CreateFontIndirect(&TempFont);
			//Link Box
			TempFont.lfHeight=12; //Font Size=8
			TempFont.lfUnderline=TRUE;
			strcpy(TempFont.lfFaceName, "MS Sans Serif");
			HFONT LinkFont=CreateFontIndirect(&TempFont);
			LinkHwnd=CreateWindow("Edit", ProjectLink, WS_CHILD|WS_VISIBLE|ES_READONLY, 0, MyHeight-16, MyWidth, 16, hwnd, NULL, (HINSTANCE)GetModuleHandle(NULL), 0);
			SendMessage(LinkHwnd, WM_SETFONT, (WPARAM)LinkFont, MAKELPARAM(TRUE, 0));
			LinkBoxProc=(TypeWinProc)SetWindowLong(LinkHwnd, GWL_WNDPROC, (LONG)AboutWinProc); //Listen for double clicks
			return 0;
		}
		case WM_PAINT:
		{
			//Prepare drawing
			PAINTSTRUCT DrawMe;
			HDC MyDC=BeginPaint(hwnd, &DrawMe);
			SetBkMode(MyDC, TRANSPARENT);
			SetTextColor(MyDC, 0x0);
			
			//Get Width of title string
			SIZE TitleStringSize;
			SelectObject(MyDC, TitleFont);
			GetTextExtentPoint32(MyDC, TitleString, sizeof(TitleString)-1, &TitleStringSize);

			//Output character strings
			TextOut(MyDC, (MyWidth-TitleStringSize.cx)/2,  0, TitleString, sizeof(TitleString));
			SelectObject(MyDC, VersionFont);
			TextOut(MyDC, (MyWidth+TitleStringSize.cx)/2,  20, VersionString, sizeof(VersionString));
			SelectObject(MyDC, ExplainFont);
			DrawText(MyDC, ExplainString, sizeof(ExplainString), (RECT*)&ExplainRect, DT_CENTER);

			//Draw signature and end drawing process
			DisplaySignature(MyDC, MyWidth-207, MyHeight-42-14);
			EndPaint(hwnd, &DrawMe);
			break;
		}
		case WM_CTLCOLORSTATIC: //Properly color the project link box
		{
			SetTextColor((HDC)wParam, GetSysColor(COLOR_HIGHLIGHT));
		    SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
			return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
		}
		case WM_CLOSE: //Destroy all opened handles
			DeleteObject(TitleFont);
			DeleteObject(VersionFont);
			DeleteObject(ExplainFont);
			DeleteObject(LinkFont);
			DestroyWindow(hwnd);
			UnregisterClass("AboutWindow", (HINSTANCE)GetModuleHandle(NULL));
			DisplaySignature(NULL, 0, 0); //Delete the signature data
			AboutHwnd=NULL; //Clear the handle so we know about window is no longer open
			break;
	}
	return DefWindowProc(hwnd,msg,wParam,lParam);
}

//Display Encoded Signature - See http://www.castledragmire.com/Posts/Project_About_Pages
//Encoding file header
#pragma warning(disable:4200) //Disable 0 length index warning
typedef struct
{
	USHORT DataSize; //Data size in bits
	UCHAR Width, Height;
	UCHAR TranspSize, TranslSize; //Largest number of bits required for a run length for Transp[arent] and Transl[ucent]
	UCHAR NumIndexes, Indexes[0]; //Number and list of indexes
} EncodedFileHeader;

__forceinline UCHAR NumBitsRequired(UINT Num) //Tests how many bits are required to represent a number
{
	UCHAR RetNum;
	_asm //Find the most significant bit
	{
		xor eax, eax //eax=0
		bsr eax, Num //Find most significant bit in eax
		mov RetNum, al
	}
	return RetNum+((UCHAR)(1<<RetNum)==Num ? 0 : 1); //Test if the most significant bit is the only one set, if not, at least 1 more bit is required
}

UCHAR _fastcall ReadBits(UCHAR* StartPointer, UINT BitStart, UINT BitSize) //Read value from Bit# BitStart after StartPointer - Assumes more than 8 bits are never read
{
	return (*(WORD*)&StartPointer[BitStart/8]>>BitStart%8)&((1<<BitSize)-1);
}

__forceinline void memsetd(const DWORD *Dest, DWORD Data, DWORD NumberOfDWORDS) //Set a DWORD at a time
{
	_asm
	{
		mov eax, Data
		mov edi, Dest
		mov ecx, NumberOfDWORDS
		rep stosd
	}
}

void DisplaySignature(HDC MyDC, int x, int y)
{
	static DWORD *Signature=NULL; //This holds the DIBSection signature

	if(!MyDC) //This is called when window is destroyed, so delete signature data
	{
		delete[] Signature;
		Signature=NULL;
		return;
	}
	else if(Signature==NULL) //Signature data has not yet been extracted, so do so
	{
		//Prepare to decode signature
			//const UCHAR BytesPerPixel=4, TranspMask=255; //32 bits per pixel (for quicker copies and such - variable not used due to changing BYTE*s to DWORD*s), and white=transparent background color - also not used anymore since we directly write in the background color
			//Load data from executable
			HGLOBAL GetData=LoadResource(NULL, FindResource(NULL, "DakSig", "Sig")); //Load the resource from the executable
			BYTE *Input=(BYTE*)LockResource(GetData); //Get the resource data

			//Prepare header and decoding data
			UINT BitOn=0; //Bit we are currently on in reading
			EncodedFileHeader H=*(EncodedFileHeader*)Input; //Save header locally so we have quick memory lookups
			DWORD *Output=Signature=new DWORD[H.Width*H.Height]; //Allocate signature memory

			//Prepare the index colors
			DWORD Indexes[17], IndexSize=NumBitsRequired(H.NumIndexes); //Save full color indexes locally so we have quick lookups, use 17 index array so we don't have to allocate memory (since we already know how many there will be), #16=transparent color
			DWORD BackgroundColor=GetSysColor(COLOR_BTNFACE), FontColor=0x067906;
			BYTE *BGC=(BYTE*)&BackgroundColor, *FC=(BYTE*)&FontColor;
			for(UINT i=0;i<16;i++) //Alpha blend the indexes
			{
				float Alpha=((EncodedFileHeader*)Input)->Indexes[i] / 255.0f;
				BYTE IndexColor[4];
				for(int n=0;n<3;n++)
					IndexColor[n]=(BYTE)(BGC[n]*Alpha + FC[n]*(1-Alpha));
				//IndexColor[3]=0; //Don't really need to worry about the last byte as it is unused
				Indexes[i]=*(DWORD*)IndexColor;
			}
			Indexes[16]=BackgroundColor; //Translucent background = window background color

		//Unroll/unencode all the pixels
		Input+=(sizeof(EncodedFileHeader)+H.NumIndexes); //Start reading input past the header
		do
		{
			UINT l; //Length (transparent and then index)
			//Transparent pixels
			memsetd(Output, Indexes[16], l=ReadBits(Input, BitOn, H.TranspSize));
			Output+=l;

			//Translucent pixels
			l=ReadBits(Input, BitOn+=H.TranspSize, H.TranslSize);
			BitOn+=H.TranslSize;
			for(i=0;i<l;i++) //Write the gray scale out to the 3 pixels, this should technically be done in a for loop, which would unroll itself anyways, but this way ReadBits+index lookup is only done once - ** Would need to be in a for loop if not using gray-scale or 24 bit output
				Output[i]=Indexes[ReadBits(Input, BitOn+i*IndexSize, IndexSize)];
			Output+=l;
			BitOn+=l*IndexSize;
		} while(BitOn<H.DataSize);
	}

	//Output the signature
	const BITMAPINFOHEADER MyBitmapInfo={sizeof(BITMAPINFOHEADER), 207, 42, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
	SetDIBitsToDevice(MyDC, x, y, MyBitmapInfo.biWidth, MyBitmapInfo.biHeight, 0, 0, 0, MyBitmapInfo.biHeight, Signature, (BITMAPINFO*)&MyBitmapInfo, DIB_RGB_COLORS);
}