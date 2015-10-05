#ifndef _WRITERAM_H
#define _WRITERAM_H

#ifndef DWORD
	#define DWORD unsigned int
#endif

class WriteRam
{
	public:
		WriteRam();
		~WriteRam();
		void* GetBegginingPointer();
		DWORD GetSizeOfFile();
		bool Write(void*, DWORD);
		bool WriteToFile(char*);
		bool ReadFromFile(char*);
};

#endif