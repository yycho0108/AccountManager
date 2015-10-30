#include <Windows.h>
#include "resource.h"

#define PWKey TEXT("Software\\YOYO\\IDPWKey")
#define ENCRYPTION

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PassCodeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool CheckError(DWORD ErrNum);


bool GetData(TCHAR* SiteName);
bool SetData(TCHAR* SiteName, TCHAR* IDName, TCHAR* PWName, TCHAR* Description);

void LoadIDList();
void Clear();

bool SaveToBackupFile();
bool ResetFromBackupFile();


HINSTANCE g_hInst;
HWND hMainDlg;
WORD RadioButton[6] = { IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4, IDC_RADIO5,IDC_RADIO6 };
HKEY Key;

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), HWND_DESKTOP, MainDlgProc);
	return 0;
}

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SendMessage(hDlg, WM_SETICON, ICON_BIG,(LPARAM)LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1)));
		hMainDlg = hDlg;
		DWORD dwDisp;
		if (!CheckError(RegCreateKeyEx(HKEY_CURRENT_USER, PWKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &Key, &dwDisp)))
			EndDialog(hDlg, 0);
		LONG PassCodeLen;
		RegQueryValue(Key, NULL, NULL, &PassCodeLen);
		TCHAR* PwBuf = new TCHAR[PassCodeLen];
		RegQueryValue(Key, NULL, PwBuf, &PassCodeLen);

		TCHAR* PASSCODE = nullptr;

		if (dwDisp == REG_CREATED_NEW_KEY)
		{
			if (MessageBox(hDlg, TEXT("DO YOU WANT TO SET A SECURITY PASSCODE TO ACCESS THE DATABASE?"), TEXT("THIS SETTING IS ONE-TIME-ONLY, AND CANNOT BE CHANGED AFTERWORDS."), MB_YESNO) == IDYES)
			{
				if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hDlg, PassCodeDlgProc, (LPARAM)&PASSCODE)!=IDOK)
					EndDialog(hDlg, 0);
				else
					RegSetValue(Key, NULL, REG_SZ, PASSCODE, NULL);
			}
			else
			{
				TCHAR NOPASS[] = TEXT("USER_NO_PASSCODE");
				RegSetValue(Key, NULL, REG_SZ, NOPASS, NULL);
			}
		}
		else
		{
			if (lstrcmp(PwBuf, TEXT("USER_NO_PASSCODE")) == 0) // no passcode
				;
			else
			{
				//ASK FOR PASSCODE
				if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hDlg, PassCodeDlgProc, (LPARAM)&PASSCODE) != IDOK)
					EndDialog(hDlg, 0);//CANCEL
				else if (lstrcmp(PwBuf, PASSCODE) != 0)
				{
					MessageBox(hDlg, TEXT("WRONG PASSCODE!!"), TEXT("ERROR"), MB_OK);
					EndDialog(hDlg, 0);
				}
				else; //RIGHT PASSCODE
			}
		}
		delete[] PASSCODE;
		delete[] PwBuf;

		SendDlgItemMessage(hDlg, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
	}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_DO:
		{
			int i;
			for (i = 0; i< _countof(RadioButton); ++i)
			{
				if (SendDlgItemMessage(hDlg, RadioButton[i], BM_GETCHECK, 0, 0) == BST_CHECKED)
					break;
			}

			switch (i)
			{
			case 0: //get Pw+Desc
			{
				//Get SITE NAME
				int Len = GetWindowTextLength(GetDlgItem(hDlg, ID_SITE));
				TCHAR* SiteName = new TCHAR[Len + 1]();
				GetWindowText(GetDlgItem(hDlg, ID_SITE), SiteName, Len+1);
				//Retrieve ID-PW
				if (!GetData(SiteName))
					MessageBox(hDlg, L"FAILED TO RETRIEVE DATA", L"ERROR", MB_OK);
				delete[] SiteName;

			}

				break;
			case 1: //set Pw+Desc
			{
				//Get User Data
				int SiteLen = GetWindowTextLength(GetDlgItem(hDlg, ID_SITE));
				TCHAR* SiteName = new TCHAR[SiteLen + 1]();
				GetWindowText(GetDlgItem(hDlg, ID_SITE), SiteName,SiteLen+1);

				int IDLen = GetWindowTextLength(GetDlgItem(hDlg, ID_ID));
				TCHAR* IDName = new TCHAR[IDLen + 1]();
				GetWindowText(GetDlgItem(hDlg, ID_ID), IDName, IDLen+1);

				int PWLen = GetWindowTextLength(GetDlgItem(hDlg, ID_PW));
				TCHAR* PWName = new TCHAR[PWLen + 1]();
				GetWindowText(GetDlgItem(hDlg, ID_PW), PWName, PWLen + 1);

				int DescLen = GetWindowTextLength(GetDlgItem(hDlg, ID_DESC));
				TCHAR* Description = new TCHAR[DescLen + 1]();
				GetWindowText(GetDlgItem(hDlg, ID_DESC), Description, DescLen+1);

				//Set Data
				if (!SetData(SiteName, IDName, PWName, Description))
					MessageBox(hDlg, L"FAILED TO SET ID", L"ERROR", MB_OK);
				else
					LoadIDList();

				delete[] SiteName;
				delete[] IDName;
				delete[] PWName;
				delete[] Description;
			}
				break;
			case 2: //Reload
				LoadIDList();
				break;
			case 3:
				if (!SaveToBackupFile())
					MessageBox(hDlg, TEXT("SAVE FAILED"), TEXT("ERROR"), MB_OK);
				break;
			case 4:
				if (!ResetFromBackupFile())
					MessageBox(hDlg, TEXT("RESET FAILED"), TEXT("ERROR"), MB_OK);
				else
					LoadIDList();
				break;
			case 5: //remove login data
			{
				int SiteLen = GetWindowTextLength(GetDlgItem(hDlg, ID_SITE));
				TCHAR* SiteName = new TCHAR[SiteLen + 1]();
				GetWindowText(GetDlgItem(hDlg, ID_SITE), SiteName, SiteLen + 1);

				if (CheckError(RegDeleteTree(Key, SiteName)))
					LoadIDList();

				delete[] SiteName;
			}
				break;
			}
		}
			break;
		case IDC_COMBO1:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				int i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
				int Len = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETLBTEXTLEN, i, 0);
				TCHAR* SiteName = new TCHAR[Len + 1];
				SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETLBTEXT, i, (LPARAM)SiteName);
				SetWindowText(GetDlgItem(hDlg, ID_SITE), SiteName);
				if (!GetData(SiteName))
					MessageBox(hDlg, TEXT("FAILED TO RETRIEVE ID"), TEXT("ERROR"), MB_OK);

				delete[] SiteName;
			}
			break;
			}
			break;
		case ID_CLEAR:
		{
			Clear();
		}
			break;
		case ID_QUIT:
			EndDialog(hDlg, 0);
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	case WM_DESTROY:
		RegCloseKey(Key);
		PostQuitMessage(0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

bool CheckError(DWORD ErrNum)
{
	if (ErrNum == ERROR_SUCCESS)
	{
		return true;
	}
	else
	{
		TCHAR ERRBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrNum, NULL, ERRBuf, _countof(ERRBuf), NULL);
		MessageBox(hMainDlg, ERRBuf, TEXT("ERROR"), MB_OK);
		return false;
	}
}


bool GetData(TCHAR* SiteName)
{
	//HAVE ID VALUE
	HKEY SiteKey;
	LSTATUS Res;
	bool success = false;

	if (CheckError(RegOpenKeyEx(Key, SiteName, 0, KEY_READ, &SiteKey)))
	{
		DWORD IDLen;
		DWORD PWLen;
		DWORD DescLen;
		RegQueryValueEx(SiteKey, TEXT("ID"), NULL, NULL, NULL, &IDLen);
		RegQueryValueEx(SiteKey, TEXT("PW"), NULL, NULL, NULL, &PWLen);
		RegQueryValueEx(SiteKey, TEXT("DESC"), NULL, NULL, NULL, &DescLen);
		TCHAR* IDVal = new TCHAR[IDLen];
		TCHAR* PWVal = new TCHAR[PWLen];
		TCHAR* DescVal = new TCHAR[DescLen];

		//DWORD Type;
		if (CheckError(RegQueryValueEx(SiteKey, TEXT("ID"), NULL, NULL, (LPBYTE)IDVal, &IDLen))
			&& CheckError(RegQueryValueEx(SiteKey, TEXT("PW"), NULL, NULL, (LPBYTE)PWVal, &PWLen))
			&& CheckError(RegQueryValueEx(SiteKey, TEXT("DESC"), NULL, NULL, (LPBYTE)DescVal, &DescLen)))
		{
			SetWindowText(GetDlgItem(hMainDlg, ID_ID), IDVal);
			SetWindowText(GetDlgItem(hMainDlg, ID_PW), PWVal);
			SetWindowText(GetDlgItem(hMainDlg, ID_DESC), DescVal);
			success = true;
		}
		delete[] IDVal;
		delete[] PWVal;
		delete[] DescVal;
	}
	RegCloseKey(SiteKey);
	return success;
}

bool SetData(TCHAR* SiteName, TCHAR* IDName, TCHAR* PWName, TCHAR* Description)
{
	//ID,PW,Desc
	HKEY SiteKey;
	DWORD dwDisp;
	bool success = false;

	if (CheckError(RegCreateKeyEx(Key, SiteName, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &SiteKey, &dwDisp)))
	{
		if (CheckError(RegSetValueEx(SiteKey, TEXT("ID"), NULL, REG_SZ, (LPBYTE)IDName, sizeof(TCHAR)*(lstrlen(IDName) + 1)))//include nullptr, multiply by TCHAR
			&& CheckError(RegSetValueEx(SiteKey, TEXT("PW"), NULL, REG_SZ, (LPBYTE)PWName, sizeof(TCHAR)*(lstrlen(PWName) + 1)))
			&& CheckError(RegSetValueEx(SiteKey, TEXT("DESC"), NULL, REG_SZ, (LPBYTE)Description, sizeof(TCHAR)*(lstrlen(Description) + 1))))
			success = true;
	}

	RegCloseKey(SiteKey);
	return success;
}

void LoadIDList()
{
	SendDlgItemMessage(hMainDlg, IDC_COMBO1, CB_RESETCONTENT, NULL, NULL);
	TCHAR SiteName[256];
	for (int i = 0; RegEnumKey(Key, i, SiteName, _countof(SiteName)) == ERROR_SUCCESS; ++i)
	{
		SendDlgItemMessage(hMainDlg, IDC_COMBO1, CB_ADDSTRING, NULL, (LPARAM)SiteName);
	}
}

void Clear()
{
	SetWindowText(GetDlgItem(hMainDlg, ID_SITE), TEXT(""));
	SetWindowText(GetDlgItem(hMainDlg, ID_ID), TEXT(""));
	SetWindowText(GetDlgItem(hMainDlg, ID_PW), TEXT(""));
	SetWindowText(GetDlgItem(hMainDlg, ID_DESC), TEXT(""));
}

struct tag_Backup
{
	enum{ A, U };
	BOOL Encoding;
	LONG PassCodeLen;
	BYTE* PassCode;
	struct tag_Header
	{
		DWORD SiteNameLen; //including null terminatnig.
		DWORD IDLen;
		DWORD PWLen;
		DWORD DescLen;
	} Header;
	//interface
	BYTE* SiteName;
	BYTE* ID;
	BYTE* PW;
	BYTE* Desc;
};

#ifdef _UNICODE
BOOL Encoding = tag_Backup::U;
#else
BOOL Encoding = tag_Backup::A;
#endif



#ifdef ENCRYPTION

struct tag_Encrypter
{
	HANDLE hSrcFile;
	HANDLE hTempFile;
	BOOL Active;
	TCHAR szTempFileName[MAX_PATH];
	TCHAR* szFileName;

	//UINT64 FileSize;
	HANDLE Encode(HANDLE& hFile, TCHAR* Path)
	{
		static bool CurState = false;

		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		hTempFile = INVALID_HANDLE_VALUE;


		//DWORD cBytes;
		hTempFile = CreateFileW((LPTSTR)szTempFileName, // file name 
			GENERIC_ALL,        // open for write 
			0,                    // do not share 
			NULL,                 // default security 
			CREATE_ALWAYS,        // overwrite existing
			FILE_FLAG_DELETE_ON_CLOSE,// normal file 
			NULL);                // no template 
		if (hTempFile == INVALID_HANDLE_VALUE)
		{
			Active = false;
		}

		hSrcFile = hFile = CreateFile(Path,              // file name 
			GENERIC_ALL,          // open for reading 
			0,                     // do not share 
			NULL,                  // default security 
			OPEN_EXISTING,         // existing file only 
			FILE_ATTRIBUTE_NORMAL, // normal file 
			NULL);                 // no template 
		if (hSrcFile == INVALID_HANDLE_VALUE)
		{
			int E = GetLastError();
			Active = false;
		}


		if (!Active) return false;
		Encode_Helper();

		FlushFileBuffers(hTempFile);
		FlushFileBuffers(hSrcFile);

		SetFilePointer(hTempFile, 0, NULL, FILE_BEGIN);
		SetFilePointer(hSrcFile, 0, NULL, FILE_BEGIN);
		
		/*
		DWORD cBytes;
		BYTE VALT;
		
		TCHAR PathBuf[256];
		lstrcpy(PathBuf, Path);
		lstrcat(PathBuf, TEXT("13"));
		HANDLE NextFile = CreateFile(PathBuf,      // file name 
			GENERIC_WRITE,          // open for reading 
			0,                     // do not share 
			NULL,                  // default security 
			OPEN_ALWAYS,         // existing file only 
			FILE_ATTRIBUTE_NORMAL, // normal file 
			NULL);
		DWORD cb2;
		while (ReadFile(hTempFile, &VALT, sizeof(VALT), &cBytes, NULL) && cBytes)
		{
			WriteFile(NextFile, &VALT, cBytes, (LPDWORD)&cb2, NULL);
		}

		SetFilePointer(hTempFile, 0, NULL, FILE_BEGIN);
		SetFilePointer(hSrcFile, 0, NULL, FILE_BEGIN);
		*/


		//MoveFileEx(szTempFileName, Path,
		//	MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);

		//hFile = hSrcFile = CreateFile(Path,              // file name 
		//	GENERIC_READ,          // open for reading 
		//	0,                     // do not share 
		//	NULL,                  // default security 
		//	OPEN_EXISTING,         // existing file only 
		//	FILE_ATTRIBUTE_NORMAL, // normal file 
		//	NULL);                 // no template 

		if (hSrcFile == INVALID_HANDLE_VALUE)
		{
			Active = false;
		}
		return hTempFile;
	}
	void Encode_Helper()
	{
		DWORD cBytes = 0;
		union BUF_TYPE
		{
			struct{
				BYTE v1 : 1;
				BYTE v2 : 1;
				BYTE v3 : 1;
				BYTE v4 : 1;
				BYTE v5 : 1;
				BYTE v6 : 1;
				BYTE v7 : 1;
				BYTE v8 : 1;
			};
			BYTE Val;
			BUF_TYPE& flip()
			{
				BUF_TYPE tmp;
				tmp.Val = Val;
				tmp.v1 = v8;
				tmp.v2 = v7;
				tmp.v3 = v6;
				tmp.v4 = v5;
				tmp.v5 = v4;
				tmp.v6 = v3;
				tmp.v7 = v2;
				tmp.v8 = v1;
				Val = tmp.Val;
				return *this;
			}
		}Buf;
		if (ReadFile(hSrcFile, &Buf, 1, &cBytes, NULL) && cBytes == 0)
			return;
		Encode_Helper();
		WriteFile(hTempFile, &Buf.flip(), 1, &cBytes, NULL);
		return;
	}
	tag_Encrypter()
	{
		Active = true;

		BOOL fSuccess = FALSE;
		DWORD dwRetVal = 0;
		UINT uRetVal = 0;

		TCHAR lpTempPathBuffer[MAX_PATH];
		dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
		{
			Active = false;
		}

		//  Generates a temporary file name. 
		uRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
			TEXT("DEMO"),     // temp file name prefix 
			0,                // create unique name 
			szTempFileName);  // buffer for name 
		if (uRetVal == 0)
		{
			Active = false;
		}

	}
	~tag_Encrypter(){ }
} Encrypter;

bool SaveToBackupFile()
{
	TCHAR FilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, FilePath);
	lstrcat(FilePath, TEXT("\\Backup.bin"));

	HANDLE hFile = CreateFile(FilePath, GENERIC_ALL, NULL, NULL, OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CheckError(GetLastError());
		return false;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (MessageBox(hMainDlg, L"Backup.bin already exists. Overwrite?", L"ERROR", MB_OKCANCEL) != IDOK)
		{
			CloseHandle(hFile);
			return false;
		}
	}

	DWORD cBytes = 0;
	WriteFile(hFile, &Encoding, sizeof(Encoding), &cBytes, NULL);

	LONG PassCodeLen;
	RegQueryValue(Key, NULL, NULL, &PassCodeLen);
	TCHAR* PassCode = new TCHAR[PassCodeLen]();
	RegQueryValue(Key, NULL, PassCode, &PassCodeLen);
	WriteFile(hFile, &PassCodeLen, sizeof(PassCodeLen), &cBytes, NULL);
	WriteFile(hFile, PassCode, PassCodeLen*sizeof(TCHAR), &cBytes, NULL);
	delete[] PassCode;


	TCHAR SiteName[256];
	for (int i = 0; RegEnumKey(Key, i, SiteName, _countof(SiteName)) == ERROR_SUCCESS; ++i)
	{
		HKEY SiteKey;
		RegOpenKey(Key, SiteName, &SiteKey);
		DWORD SiteNameLen = (lstrlen(SiteName) + 1)*sizeof(TCHAR); //IN BYTES, include null end

		DWORD IDLen;
		DWORD PWLen;
		DWORD DescLen;

		RegQueryValueEx(SiteKey, TEXT("ID"), NULL, NULL, 0, &IDLen);
		RegQueryValueEx(SiteKey, TEXT("PW"), NULL, NULL, 0, &PWLen);
		RegQueryValueEx(SiteKey, TEXT("DESC"), NULL, NULL, 0, &DescLen);

		TCHAR* IDVal = new TCHAR[IDLen];
		TCHAR* PWVal = new TCHAR[PWLen];
		TCHAR* DescVal = new TCHAR[DescLen];

		if (CheckError(RegQueryValueEx(SiteKey, TEXT("ID"), NULL, NULL, (LPBYTE)IDVal, &IDLen))
			&& CheckError(RegQueryValueEx(SiteKey, TEXT("PW"), NULL, NULL, (LPBYTE)PWVal, &PWLen))
			&& CheckError(RegQueryValueEx(SiteKey, TEXT("DESC"), NULL, NULL, (LPBYTE)DescVal, &DescLen)))
		{
			WriteFile(hFile, &SiteNameLen, sizeof(SiteNameLen), &cBytes, NULL);
			WriteFile(hFile, &IDLen, sizeof(IDLen), &cBytes, NULL);
			WriteFile(hFile, &PWLen, sizeof(PWLen), &cBytes, NULL);
			WriteFile(hFile, &DescLen, sizeof(DescLen), &cBytes, NULL);

			WriteFile(hFile, SiteName, SiteNameLen, &cBytes, NULL);
			WriteFile(hFile, IDVal, IDLen, &cBytes, NULL);
			WriteFile(hFile, PWVal, PWLen, &cBytes, NULL);
			WriteFile(hFile, DescVal, DescLen, &cBytes, NULL);
		}
		else
		{
			delete[] IDVal;
			delete[] PWVal;
			delete[] DescVal;
			RegCloseKey(SiteKey);
			CloseHandle(hFile);
			return false;
		}
		delete[] IDVal;
		delete[] PWVal;
		delete[] DescVal;
		RegCloseKey(SiteKey);
	}

	//Encrypt
	FlushFileBuffers(hFile);
	HANDLE hTemp = Encrypter.Encode(hFile, FilePath);

	UINT64 BigBuf;
	DWORD cBytes2 = 0;

	while (ReadFile(hTemp, &BigBuf, sizeof(BigBuf), &cBytes, NULL) && cBytes != 0)
	{
		WriteFile(hFile, &BigBuf, cBytes, &cBytes2, NULL);
	}
	//파일 복사를 하시길?
	CloseHandle(hFile);
	CloseHandle(hTemp);
	return true;
}

bool ResetFromBackupFile()
{
	TCHAR FilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, FilePath);
	lstrcat(FilePath, TEXT("\\Backup.bin"));

	HANDLE hFile = CreateFile(FilePath, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CheckError(GetLastError());
		return false;
	}
	HANDLE hTemp = Encrypter.Encode(hFile, FilePath); // hTemp = translated table
	CloseHandle(hFile);//not much of use now.

	DWORD cBytes = 0;
	BOOL File_Encoding = 0;
	if (!ReadFile(hTemp, &File_Encoding, sizeof(File_Encoding), &cBytes, NULL))
	{
		CheckError(GetLastError());
		CloseHandle(hTemp);
		return false;
	}
	if (File_Encoding != Encoding)
	{
		MessageBox(hMainDlg, TEXT("Incompatible Backup File"), TEXT("ERROR"), MB_OK);
		CloseHandle(hTemp);
		return false;
	}

	LONG PassCodeLen;
	ReadFile(hTemp, &PassCodeLen, sizeof(PassCodeLen), &cBytes, NULL);
	TCHAR* PassCode = new TCHAR[PassCodeLen];
	ReadFile(hTemp, PassCode, PassCodeLen*sizeof(TCHAR), &cBytes, NULL);

	RegSetValue(Key, NULL, REG_SZ, PassCode, NULL);

	//set data back
	delete[] PassCode;


	tag_Backup::tag_Header Header;

	TCHAR* SiteName;
	TCHAR* IDVal;
	TCHAR* PWVal;
	TCHAR* DescVal;
	BOOL b;
	while (ReadFile(hTemp, &Header, sizeof(Header), &cBytes, NULL))
	{
		if (cBytes == 0) break; //no more to read
		SiteName = new TCHAR[Header.SiteNameLen]();
		IDVal = new TCHAR[Header.IDLen]();
		PWVal = new TCHAR[Header.PWLen]();
		DescVal = new TCHAR[Header.DescLen]();
		ReadFile(hTemp, SiteName, Header.SiteNameLen, &cBytes, NULL);
		ReadFile(hTemp, IDVal, Header.IDLen, &cBytes, NULL);
		ReadFile(hTemp, PWVal, Header.PWLen, &cBytes, NULL);
		ReadFile(hTemp, DescVal, Header.DescLen, &cBytes, NULL);
		SetData(SiteName, IDVal, PWVal, DescVal);
		delete[] SiteName;
		delete[] IDVal;
		delete[] PWVal;
		delete[] DescVal;
	}

	CloseHandle(hTemp);
	return true;
}

#else
bool SaveToBackupFile()
{
	TCHAR FilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, FilePath);
	lstrcat(FilePath, TEXT("\\Backup.bin"));

	HANDLE hFile = CreateFile(FilePath, GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CheckError(GetLastError());
		return false;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (MessageBox(hMainDlg, L"Backup.bin already exists. Overwrite?", L"ERROR", MB_OKCANCEL) != IDOK)
		{
			CloseHandle(hFile);
			return false;
		}
	}

	DWORD cBytes = 0;
	WriteFile(hFile, &Encoding, sizeof(Encoding), &cBytes, NULL);
	
	LONG PassCodeLen;
	RegQueryValue(Key, NULL, NULL, &PassCodeLen);
	TCHAR* PassCode = new TCHAR[PassCodeLen]();
	RegQueryValue(Key, NULL, PassCode, &PassCodeLen);
	WriteFile(hFile, &PassCodeLen, sizeof(PassCodeLen), &cBytes, NULL);
	WriteFile(hFile, PassCode, PassCodeLen, &cBytes, NULL);
	delete[] PassCode;


	TCHAR SiteName[256];
	for (int i = 0; RegEnumKey(Key, i, SiteName, _countof(SiteName)) == ERROR_SUCCESS; ++i)
	{
		HKEY SiteKey;
		RegOpenKey(Key, SiteName, &SiteKey);
		DWORD SiteNameLen = (lstrlen(SiteName)+1)*sizeof(TCHAR); //IN BYTES, include null end
		
		DWORD IDLen;
		DWORD PWLen;
		DWORD DescLen;

		RegQueryValueEx(SiteKey, TEXT("ID"), NULL, NULL,0, &IDLen);
		RegQueryValueEx(SiteKey, TEXT("PW"), NULL, NULL,0, &PWLen);
		RegQueryValueEx(SiteKey, TEXT("DESC"), NULL, NULL,0, &DescLen);

		TCHAR* IDVal = new TCHAR[IDLen];
		TCHAR* PWVal = new TCHAR[PWLen];
		TCHAR* DescVal = new TCHAR[DescLen];

		if (CheckError(RegQueryValueEx(SiteKey, TEXT("ID"), NULL, NULL, (LPBYTE)IDVal, &IDLen))
			&& CheckError(RegQueryValueEx(SiteKey, TEXT("PW"), NULL, NULL, (LPBYTE)PWVal, &PWLen))
			&& CheckError(RegQueryValueEx(SiteKey, TEXT("DESC"), NULL, NULL, (LPBYTE)DescVal, &DescLen)))
		{
			WriteFile(hFile, &SiteNameLen, sizeof(SiteNameLen), &cBytes, NULL);
			WriteFile(hFile, &IDLen, sizeof(IDLen), &cBytes, NULL);
			WriteFile(hFile, &PWLen, sizeof(PWLen), &cBytes, NULL);
			WriteFile(hFile, &DescLen, sizeof(DescLen), &cBytes, NULL);

			WriteFile(hFile, SiteName, SiteNameLen, &cBytes, NULL);
			WriteFile(hFile, IDVal, IDLen, &cBytes, NULL);
			WriteFile(hFile, PWVal, PWLen, &cBytes, NULL);
			WriteFile(hFile, DescVal, DescLen, &cBytes, NULL);
		}
		else
		{
			delete[] IDVal;
			delete[] PWVal;
			delete[] DescVal;
			RegCloseKey(SiteKey);
			CloseHandle(hFile);
			return false;
		}
		delete[] IDVal;
		delete[] PWVal;
		delete[] DescVal;
		RegCloseKey(SiteKey);
	}
	CloseHandle(hFile);
	return true;
}

bool ResetFromBackupFile()
{
	TCHAR FilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, FilePath);
	lstrcat(FilePath, TEXT("\\Backup.bin"));

	HANDLE hFile = CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CheckError(GetLastError());
		return false;
	}
	DWORD cBytes = 0;
	BOOL File_Encoding = 0;
	if (!ReadFile(hFile, &File_Encoding, sizeof(File_Encoding), &cBytes, NULL))
	{
		CheckError(GetLastError());
		CloseHandle(hFile);
		return false;
	}
	if (File_Encoding != Encoding)
	{
		MessageBox(hMainDlg, TEXT("Incompatible Backup File"), TEXT("ERROR"), MB_OK);
		CloseHandle(hFile);
		return false;
	}

	LONG PassCodeLen;
	ReadFile(hFile, &PassCodeLen, sizeof(PassCodeLen), &cBytes, NULL);
	TCHAR* PassCode = new TCHAR[PassCodeLen];
	ReadFile(hFile, PassCode, PassCodeLen, &cBytes, NULL);

	RegSetValue(Key, NULL, REG_SZ, PassCode, NULL);

	//set data back
	delete[] PassCode;


	tag_Backup::tag_Header Header;

	TCHAR* SiteName;
	TCHAR* IDVal;
	TCHAR* PWVal;
	TCHAR* DescVal;
	while (ReadFile(hFile, &Header, sizeof(Header), &cBytes, NULL))
	{
		if (cBytes == 0) break; //no more to read
		SiteName = new TCHAR[Header.SiteNameLen]();
		IDVal = new TCHAR[Header.IDLen]();
		PWVal = new TCHAR[Header.PWLen]();
		DescVal = new TCHAR[Header.DescLen]();
		ReadFile(hFile, SiteName, Header.SiteNameLen, &cBytes, NULL);
		ReadFile(hFile, IDVal, Header.IDLen, &cBytes, NULL);
		ReadFile(hFile, PWVal, Header.PWLen, &cBytes, NULL);
		ReadFile(hFile, DescVal, Header.DescLen, &cBytes, NULL);
		SetData(SiteName, IDVal, PWVal, DescVal);
		delete[] SiteName;
		delete[] IDVal;
		delete[] PWVal;
		delete[] DescVal;
	}

	CloseHandle(hFile);
	return true;
}
#endif


BOOL CALLBACK PassCodeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static TCHAR** PASSCODEADDR;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		PASSCODEADDR = (TCHAR**)lParam;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			int Len = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT1));
			*PASSCODEADDR = new TCHAR[Len + 1];
			GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), *PASSCODEADDR, Len + 1);
			EndDialog(hDlg, IDOK);
		}
		break;
		case IDCANCEL:
		{
			*PASSCODEADDR = nullptr;
			EndDialog(hDlg, IDCANCEL);
		
		}break;
		}
		break;
	case WM_CLOSE:
		*PASSCODEADDR = nullptr;
		EndDialog(hDlg, IDCANCEL);
		break;
	case WM_DESTROY:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}