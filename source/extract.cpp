/*
You will need the zlib libraries at www.zlib.org.  Only 6 of the .c source files are used, and I included them in the zip file.

There where many jury rigs I threw in to make as many of the pictures as I could work. The actual compress files found in the data.bin contain a type of format I believe to be called .cmp (though that could be the compression format too, heh).  All of these files contain within them multiple other files, though usually just 1.  This program is designed to read a file with 1 picture inside.  Files with multiple pictures inside will cause palette/color and other problems (see stfroll1 & xddwal49), but they will still display the basic picture.  The other type of file besides .bmp that the .cmp contains are .max files, which I am unsure of what they do, and have no real relevance to what I am trying to accomplish.  It would be pretty simple to incorporate for multiple files, just have to point which palettes go with which picture.

The main jurry rig is contained on this line:
static BYTE PalettePicSeperator[]={0x00,0x03,0xCC,0xCC,0xFE,0xFE,0xFE,0x00,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xCD,0xCD,0xFE,0xFE,0xFE,0xFE,0xFE};
This is the sequence of bytes the program looks for to know when it has found the boundary between the palette and the picture.  This area also contains some information on the format of the pictures, which I did not have time to study.  Some of the bytes in this sequence seem to remain constant for 99% of the files, and only change for a few, like byte 5, 10, 14, 17, 18,30,31,33.  Other bytes such as 22,29,32 seem to contain useful information like palette size and bitmap width.  The specific bytes are commented on the line above the "PalettePicSeperator[]" line.

ConsoleStreamBuf & WriteRam are libraries I coded for personal use.  I included the WriteRam library file, but please don't ask for any further information on these files.
  Jeffrey Riaboy (Dakusan)
*/


//General, compression, and console functions
#include <windows.h>
#include "zlib/zlib.h"
//#include <consolestreambuf.h>

//File IO Functions
#include "WriteRam.h"
#include <io.h>
#include <fcntl.h>

//Structure to holds all positions of files in the index file
struct CompressedFile
{
	char *FileName;
	DWORD FileStart, FileSize;
	CompressedFile *NextFile;
};

//My Functions...
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool ParseDataFile(char *FullPathFileName);
DWORD LoadRawFromDataFile(CompressedFile *TheFileNode, BYTE *UncompressedBuffer, int FileNum=0, BYTE *CompressedBuffer=NULL);
bool LoadPictureFromFile(char *FileToOpen);
bool LoadPictureFromRaw(BYTE *FileContents, DWORD FileSize);

//Global variables
#define MaxNumberOfColors 256
BYTE *BitmapContents=NULL;
BITMAPINFO BitmapInfo[sizeof(BITMAPINFO)+MaxNumberOfColors];
CompressedFile *FirstFile=NULL;

//Extra stuff that I threw in for a quick gui before releasing thats not really optimized
#include <stdio.h>
DWORD ScreenWidth=800,ScreenHeight=600, WidthScale=5;
HWND MainHwnd;
char *DataFileName="DATA.BIN";
char DataFilePath[255];
enum {ItemList, SaveCompress, SaveRaw, SaveBitmap};

inline DWORD SearchForMarker(BYTE* Data, BYTE *DataToSearchFor, DWORD SizeOfData, DWORD SizeOfSearchData) //Search for a string marker in a file
{
	bool Found=false;
	DWORD p;
	for(DWORD i=0;i<(int)(SizeOfData-SizeOfSearchData) && !Found;i++)
		if(*(DWORD*)&Data[i]==*(DWORD*)DataToSearchFor)
			for(p=4,Found=true;p<SizeOfSearchData && Found;p++)
				if(Data[i+p]!=DataToSearchFor[p] && DataToSearchFor[p]!=0xFE)
					Found=false;
	return Found ? i : 0;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine,int nCmdShow)
{
	//Get the data file
	GetCurrentDirectory(255, DataFilePath);
	strcat(DataFilePath, "\\");
	strcat(DataFilePath, DataFileName);

	//Init the console
	//consolestreambuf dbgbuf(1);
	//cout=&dbgbuf;
	
	//Init Bitmap Info structure
	BITMAPINFOHEADER NewHeader={sizeof(BITMAPINFOHEADER),0,0,1,8,BI_RGB, 0, NULL, NULL, MaxNumberOfColors, MaxNumberOfColors};
	BitmapInfo->bmiHeader=NewHeader;

	//Create the main window
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0L, 0L, hInst, NULL, LoadCursor(NULL,IDC_ARROW), (HBRUSH)GetStockObject(COLOR_BACKGROUND), NULL, "HackStract", NULL };
	if(RegisterClassEx(&wc)==0)
		return false;
	RECT AdjustWindow={0,0,ScreenWidth,ScreenHeight};
	AdjustWindowRect(&AdjustWindow, WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX, false);
	MainHwnd = CreateWindow("HackStract", "Hack Picture Extractor (HackStract)", WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX, 0, 0, AdjustWindow.right-AdjustWindow.left, AdjustWindow.bottom-AdjustWindow.top, NULL, NULL, hInst, NULL);
	ShowWindow(MainHwnd, SW_SHOW);
    UpdateWindow(MainHwnd);

	//Create the main window's controls
	CreateWindow("ListBox",NULL, LBS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_VSCROLL| WS_BORDER,650,0,150,500,MainHwnd,(HMENU)ItemList,hInst,0);
	CreateWindow("Button","Save Original Compressed (GZ)",BS_PUSHBUTTON | BS_MULTILINE | WS_CHILD | WS_VISIBLE ,650,500,150,33,MainHwnd,(HMENU)SaveCompress,hInst,0);
	CreateWindow("Button","Save Original File (CMP)",BS_PUSHBUTTON | BS_MULTILINE | WS_CHILD | WS_VISIBLE ,650,533,150,33,MainHwnd,(HMENU)SaveRaw,hInst,0);
	CreateWindow("Button","Save Extracted Contents (BMP)",BS_PUSHBUTTON | BS_MULTILINE | WS_CHILD | WS_VISIBLE ,650,566,150,33,MainHwnd,(HMENU)SaveBitmap,hInst,0);

	//Open the master data file
	if(!ParseDataFile(DataFilePath))
		return S_FALSE;
	SendDlgItemMessage(MainHwnd, ItemList, LB_SETCURSEL, 0, NULL);

	//Main program loop, listen for events
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	SendMessage(MainHwnd, WM_CHAR, NULL, NULL);
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//Clear out remaining info
	UnregisterClass("HackStract", hInst);
	CompressedFile *CurrentFile=FirstFile, *NextFile;
	while(CurrentFile)
	{
		NextFile=CurrentFile->NextFile;
		delete[] CurrentFile->FileName;
		delete CurrentFile;
		CurrentFile=NextFile;
	}
	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const int TenMegs=10*1024*1024;
	switch(msg)
	{
		case WM_PAINT:
		{
			//Blit the picture to the screen
			if(!BitmapContents)
				break;
			HDC MainDC=GetDC(hWnd);
			BitBlt(MainDC, 0, 0, ScreenWidth-150, ScreenHeight, MainDC, 0, 0, WHITENESS);
			StretchDIBits(MainDC,0,0,BitmapInfo->bmiHeader.biWidth,BitmapInfo->bmiHeader.biHeight,0,0,BitmapInfo->bmiHeader.biWidth,BitmapInfo->bmiHeader.biHeight,BitmapContents,BitmapInfo,DIB_RGB_COLORS,SRCCOPY);
			ReleaseDC(hWnd, MainDC);
			break;
		}
		case WM_COMMAND:
		{
			CompressedFile *TheFile=(CompressedFile*)SendDlgItemMessage(hWnd, ItemList, LB_GETITEMDATA, SendDlgItemMessage(hWnd, ItemList, LB_GETCURSEL, NULL, NULL), NULL);
			switch(HIWORD(wParam))
			{
				case BN_CLICKED: //If one of the buttons is clicked, save accordingly
				{
					WriteRam SaveFile;
					char SaveBufferName[255];
					GetCurrentDirectory(255, SaveBufferName);
					strcat(SaveBufferName, "\\");
					strcat(SaveBufferName, TheFile->FileName);

					switch(LOWORD(wParam))
					{
						case SaveCompress: //Load compressed data to a file
						{
							//We need to rewind a little bit because uncompression normally doesnt need the headers, so our file pointer starts after the headers
							BYTE *ReadIt=new BYTE[TenMegs];
							DWORD StartRead=TheFile->FileStart>=50 ? 50 : TheFile->FileStart;
							DWORD FileNum=_open(DataFileName, _O_RDONLY|_O_BINARY);
							_lseek(FileNum, TheFile->FileStart-StartRead, SEEK_SET);
							_read(FileNum, ReadIt, TheFile->FileSize+StartRead);
							_close(FileNum);

							//Seek backwards for 4 zero bytes and assume file starts there
							for(int i=StartRead-1;i>=0;i--)
								if(!*(DWORD*)&ReadIt[i])
									break;
							i+=4;

							//Save to file
							strcat(SaveBufferName, ".gz");
							SaveFile.Write(ReadIt+i, TheFile->FileSize+StartRead-i);
							delete[] ReadIt;
							break;
						}
						case SaveRaw: //Uncompress and save
						{
							BYTE *ReadIt=new BYTE[TenMegs];
							DWORD UncompressedSize=LoadRawFromDataFile(TheFile, ReadIt);
							strcat(SaveBufferName, ".cmp");
							SaveFile.Write(ReadIt, UncompressedSize);
							delete[] ReadIt;
							break;
						}
						case SaveBitmap: //Save directly from our BitmapInfo, already decompressed
						{
							strcat(SaveBufferName, ".bmp");
							DWORD SizeOfHeader=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFO)+MaxNumberOfColors*sizeof(DWORD), SizeOfData=BitmapInfo->bmiHeader.biWidth*BitmapInfo->bmiHeader.biHeight;
							BITMAPFILEHEADER FileHeader={*(WORD*)"BM", SizeOfHeader+SizeOfData,0,0,SizeOfHeader};
							SaveFile.Write(&FileHeader, sizeof(BITMAPFILEHEADER));
							SaveFile.Write(&BitmapInfo, sizeof(BITMAPINFO)+MaxNumberOfColors*sizeof(DWORD));
							SaveFile.Write(BitmapContents, BitmapInfo->bmiHeader.biWidth*BitmapInfo->bmiHeader.biHeight);
							break;
						}
					}
					SaveFile.WriteToFile(SaveBufferName);
					SetFocus(hWnd);
					break;
				}
				case LBN_SELCHANGE: //Select the clicked picture and show
					if(LOWORD(wParam)==ItemList && lParam!=1) //Make sure that the item wasn't selected through a spacebar press
					{
						BYTE *ReadIt=new BYTE[TenMegs];
						DWORD UncompressedSize=LoadRawFromDataFile(TheFile, ReadIt);
						if(!LoadPictureFromRaw(ReadIt, UncompressedSize))
							MessageBox(NULL, "Unknown headers, or no bitmaps found", NULL, MB_OK);
						delete[] ReadIt;
						SendMessage(hWnd, WM_PAINT, NULL, NULL);
						SetFocus(hWnd); //Set focus back to main window so spacebar works
					}
			}
			break;
		}
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_CHAR:
			if((wParam==' ' || (wParam>='1' && wParam<='9'))&& msg==WM_CHAR) //Show the next readable picture if space is pressed
			{
				//Go to the next valid file
				DWORD CurSel=SendDlgItemMessage(hWnd, ItemList, LB_GETCURSEL, NULL, NULL);
				if(wParam==' ')
					CurSel++;
				else
					WidthScale=wParam-'0';
				CompressedFile *FileOn=(CompressedFile*)SendDlgItemMessage(hWnd, ItemList, LB_GETITEMDATA, CurSel, NULL);
				if((DWORD)FileOn==0xFFFFFFFF)
					break;

				BYTE *ReadIt=new BYTE[TenMegs], *ReadFile=new BYTE[TenMegs];
				DWORD UncompressedSize;
				DWORD TheFile=_open(DataFileName, _O_RDONLY|_O_BINARY);
				do //Read the compressed data until a valid picture is found
				{
					UncompressedSize=LoadRawFromDataFile(FileOn, ReadIt, TheFile, ReadFile);
					FileOn=FileOn->NextFile;
					SendDlgItemMessage(hWnd, ItemList, LB_SETCURSEL, CurSel++, 1);
				}
				while(!UncompressedSize || !LoadPictureFromRaw(ReadIt, UncompressedSize));
				_close(TheFile);
				delete[] ReadIt;
				delete[] ReadFile;
				SendMessage(hWnd, WM_PAINT, NULL, NULL);
			}
			else //Pass keyboard command onto the listbox
				SendDlgItemMessage(hWnd, ItemList, msg, wParam, lParam);
			break;
		case WM_DESTROY:
		case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool ParseDataFile(char *FullPathFileName)
{
	//Tell the user whats going on
	char ParsingStats[255];
	static RECT TheDims={0,0,ScreenWidth,ScreenHeight};
	HDC MainDC=GetDC(MainHwnd);
	sprintf(ParsingStats, "Loading main data file at %s", DataFilePath);
	BitBlt(MainDC, 0, 0, ScreenWidth, ScreenHeight, MainDC, 0, 0, WHITENESS);
	TextOut(MainDC, 200, 290, ParsingStats, strlen(ParsingStats));

	//Read in data file
	WriteRam DataFile;
	DataFile.ReadFromFile(FullPathFileName);
	BYTE *Content=(BYTE*)DataFile.GetBegginingPointer();
	DWORD FileSize=DataFile.GetSizeOfFile(), FileOn=0, TotalSize=0;
	if(!FileSize)
	{
		MessageBox(NULL, "DATA.BIN does not seem to exist in the current directory, or is inaccessible, exiting.", NULL, MB_OK);
		return false;
	}
	BitBlt(MainDC, 0, 0, 1600, 1200, MainDC, 0, 0, WHITENESS);

	//Loop through all files inside the data file
	DWORD CurrentFile=0;
	CompressedFile *CurrentFileNode=NULL;
	do
	{
		//Find where next compressed file starts so we can find the end of the current one
		static BYTE FileHeader[]={0x1F,0x8B,0x08,0x08,0xFE,0xFE,0xFE,0xFE,0x00,0x0B};
		bool FindCmp=false;
		DWORD NextFile=CurrentFile+sizeof(FileHeader);
		while(!FindCmp && NextFile)
		{
			DWORD FindTheNextFile=SearchForMarker(&Content[NextFile], FileHeader, FileSize-NextFile, sizeof(FileHeader));
			NextFile=FindTheNextFile ? NextFile+FindTheNextFile-1 : 0;

			for(DWORD p=NextFile+sizeof(FileHeader);p<NextFile+sizeof(FileHeader)+15 && !FindCmp;p++)
				if(*(DWORD*)&Content[p]==*(DWORD*)".cmp")
					FindCmp=true;
		}

		//Find when current files ends (Leave 1 zero deliminator at the end of the compressed file)
		DWORD FileEnd=NextFile ? NextFile : FileSize-1;
		while(Content[--FileEnd]==0);
			FileEnd+=2;

		//Update extraction statistics
		FileOn++;
		TotalSize+=FileEnd-CurrentFile;

		//Create linked list node
		CompressedFile *NewFileNode=new CompressedFile;
		if(!FirstFile)
			CurrentFileNode=FirstFile=NewFileNode;
		else
		{
			CurrentFileNode->NextFile=NewFileNode;
			CurrentFileNode=NewFileNode;
		}
		CurrentFileNode->FileStart=CurrentFile;
		CurrentFileNode->FileSize=FileEnd-CurrentFile;

		//Extract compressed file - init section
		z_stream TheStream={Content+CurrentFileNode->FileStart, CurrentFileNode->FileSize, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL};
		CurrentFile=NextFile;
		if(inflateInit2_(&TheStream, -MAX_WBITS, (const char*)"1.2.1", sizeof(TheStream)))
			continue; //ERROR!

		//Check the header
	    if(*(WORD*)TheStream.next_in==0x8b1f) //Check for gzip magic header
		{
			//Retrieve method & flags
			const enum {ASCII_FLAG=0x1, HEAD_CRC=0x2, EXTRA_FIELD=0x4, ORIG_NAME=0x8, COMMENT=0x10, RESERVED=0xE0}; //Bit order: file probably ascii text, header CRC present, extra field present, original file name present, file comment present, reserved, reserved, reserved
			DWORD flags=TheStream.next_in[3];
			if (TheStream.next_in[2]!=Z_DEFLATED || (flags & RESERVED)!=0)
				continue; //ERROR!

			//Move pointer forward from magic header and method+flag, and discard excess flags
			TheStream.avail_in-=10;
			TheStream.next_in+=10;

			//Clear the rest of the headers
			if(flags&EXTRA_FIELD) //Skip the extra field
			{
				TheStream.avail_in-=2+*(WORD*)TheStream.next_in;
				TheStream.next_in+=2+*(WORD*)TheStream.next_in;
			}
			if(flags&ORIG_NAME) //Get the original filename
			{
				DWORD StrLen=strlen((char*)TheStream.next_in)+1;
				CurrentFileNode->FileName=new char[StrLen-4+1]; //Take off extension
				memcpy(CurrentFileNode->FileName, TheStream.next_in, StrLen-4);
				CurrentFileNode->FileName[StrLen-5]=0;
				TheStream.next_in+=StrLen;
				TheStream.avail_in-=StrLen;
			}
			if(flags&COMMENT) //Skip the .gz file comment
				while(*(++TheStream.next_in) && --TheStream.avail_in);
			if(flags&HEAD_CRC) //Skip the header crc
			{
				TheStream.avail_in-=2;
				TheStream.next_in+=2;
			}
			if(!TheStream.avail_in) //Make sure we are not at the end of the file
				continue; //ERROR!
		}
		CurrentFileNode->FileStart+=(CurrentFileNode->FileSize-TheStream.avail_in);
		CurrentFileNode->FileSize=TheStream.avail_in;
		inflateEnd(&TheStream);

		//Add to the listbox
		SendDlgItemMessage(MainHwnd, ItemList, LB_ADDSTRING, NULL, (DWORD)CurrentFileNode->FileName);
		SendDlgItemMessage(MainHwnd, ItemList, LB_SETITEMDATA, SendDlgItemMessage(MainHwnd, ItemList, LB_GETCOUNT, NULL, NULL)-1, (DWORD)CurrentFileNode);

		//Tell the user the data file parsing statistics
		sprintf(ParsingStats, "Parsing data file (%.2f%%), %u files found   ", (double)NextFile/FileSize*100, FileOn);
		TextOut(MainDC,200,290,ParsingStats,strlen(ParsingStats));
	}
	while(CurrentFile);
	ReleaseDC(MainHwnd, MainDC);
	CurrentFileNode->NextFile=NULL;
	return true;
}

DWORD LoadRawFromDataFile(CompressedFile *TheFileNode, BYTE *UncompressedBuffer, int FileNum, BYTE *CompressedBuffer)
{
	//Load up temporary data if not passed in
	bool TempBuffer=false, TempFile=false;
	const int TenMegs=10*1024*1024;
	if(!CompressedBuffer)
	{
		TempBuffer=true;
		CompressedBuffer=new BYTE[TenMegs];
	}
	if(!FileNum)
	{
		TempFile=true;
		FileNum=_open(DataFileName, _O_RDONLY|_O_BINARY);
	}

	//Read in compressed data
	_lseek(FileNum, TheFileNode->FileStart, SEEK_SET);
	_read(FileNum, CompressedBuffer, TheFileNode->FileSize);

	//Close temporary file
	if(TempFile)
		_close(FileNum);

	//Uncompress data
	z_stream TheStream={CompressedBuffer, TheFileNode->FileSize, 0, UncompressedBuffer, TenMegs, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL};
	if(inflateInit2_(&TheStream, -MAX_WBITS, (const char*)"1.2.1", sizeof(TheStream)) || inflate(&TheStream, Z_FINISH)!=1)
	{
		if(CompressedBuffer)
			delete[] CompressedBuffer;
		return 0;
	}
	inflateEnd(&TheStream);

	//Delete temporary buffer
	if(TempBuffer)
		delete[] CompressedBuffer;

	return TheStream.total_out; //Return uncompressed data size
}

bool LoadPictureFromFile(char *FileToOpen)
{
	//Open file for reading and storing into memory
	WriteRam TheFile;
	TheFile.ReadFromFile(FileToOpen);
	return LoadPictureFromRaw((BYTE*)TheFile.GetBegginingPointer(), TheFile.GetSizeOfFile());
}

bool LoadPictureFromRaw(BYTE* FileContents, DWORD FileSize)
{
	//Find palette/data seperator						  3D/7D?	                0/2                 0/2            0/1D  0/1                Pal Mip                           Width 0/3 0/60 Wid2 0/40
	static BYTE PalettePicSeperator[]={0x00,0x03,0xCC,0xCC,0xFE,0xFE,0xFE,0x00,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xCD,0xCD,0xFE,0xFE,0xFE,0xFE,0xFE};
	static BYTE FileFooter[]={0x05,0x00,0xCC,0xCC,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0xFF,0xCC,0xCC,0x01,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF};
	DWORD Marker=SearchForMarker(FileContents, PalettePicSeperator, FileSize, sizeof(PalettePicSeperator));
	
	if(Marker) //If seperator is found, display file
	{
		/*//Debugging Output Markers
		cout.setf(ios::hex | ios::uppercase);
		cout.fill(' ');
		for(DWORD q=0; q<sizeof(PalettePicSeperator);q++)
			if(PalettePicSeperator[q]==0xFE)
			{
				cout.width(3);
				cout << (int)FileContents[Marker+q-1];
			}*/

		//Find where the data/palette start and the dimensions
		DWORD NumberOfColors=(FileContents[Marker+20]==0x13 ? 256 : 16);
		DWORD PaletteStart=Marker-NumberOfColors*sizeof(DWORD)-1, FileStart=Marker+sizeof(PalettePicSeperator)+2;
		DWORD Width=(FileContents[Marker+27]-(!!FileContents[Marker+30]*0x40))/0x40;
		Width=0x80<<(Width/2+Width%2);
		if(FileContents[Marker+21]) //Mipmap
			Width/=2;
		Width=Width>>(5-WidthScale) | Width<<(WidthScale-5);; //Jury rig to show other hights

		DWORD Height=(FileSize-sizeof(FileFooter))-FileStart;

		//**Start setting up for multiple pictures?
		//if(Height%Width)
		//	MessageBox(NULL, "Invalid or something", NULL, MB_OK);
		Height/=Width;

		//cout << "  " << Height << endl;

		//Set bitmap information
		BitmapInfo->bmiHeader.biWidth=Width;
		BitmapInfo->bmiHeader.biHeight=Height;
		if(BitmapContents) delete[] BitmapContents;
		BitmapContents=new BYTE[Width*Height];

		//Copy data and palette information into data structures
		ZeroMemory(&BitmapInfo->bmiColors, MaxNumberOfColors*sizeof(DWORD));
		CopyMemory(&BitmapInfo->bmiColors, FileContents+PaletteStart, NumberOfColors*sizeof(DWORD));
		if(NumberOfColors==16)
			ZeroMemory(((DWORD*)&BitmapInfo->bmiColors)+NumberOfColors, (MaxNumberOfColors-NumberOfColors)*sizeof(DWORD));
		CopyMemory(BitmapContents, FileContents+FileStart, Width*Height);

		//Flip from BGR structure to RGB
		for(int i=0;i<256;i++)
		{
			DWORD *Color=(DWORD*)&BitmapInfo->bmiColors[i];
			*Color=((*Color>>16)&0xFF)|(*Color&0xFF00)|((*Color<<16)&0xFF0000);
		}
	}
	return (Marker!=0);
}