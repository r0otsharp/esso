#include <windows.h>
#include <ntsecapi.h>
#include <string.h>
#include <winhttp.h>
#include <ErisADFilter.h>
#include <stdio.h>
#include <wincrypt.h>


extern HANDLE getModuleHandle();
BOOL debug = FALSE;

void DECLSPEC_EXPORT setQueueDebug(BOOL bDebug) {
	debug = bDebug;
}

void displayErrorMessage(DWORD dw) {
	LPWSTR pstr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dw, 0, (LPWSTR) &pstr, 0, NULL);
	if (pstr == NULL)
		wprintf(L"Unknown error: %d\n",
		dw);
		else
		wprintf (L"Unable to send message: %s\n",
				pstr);
		LocalFree(pstr);
	}

void getSpoolFileName(PWSTR lpszBuffer, DWORD dwSize) {
	// Obtener el nombre del modulo
	WCHAR wchModuleName[_MAX_PATH];
	lpszBuffer[0] = L'\0';
	DWORD dwModuleName = sizeof wchModuleName;
	if (dwModuleName == GetModuleFileNameW((HMODULE) getModuleHandle(), wchModuleName,
			dwModuleName)) {
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return;
	}

	WCHAR** lppPart = { NULL };
	if (GetFullPathNameW(wchModuleName, dwSize, lpszBuffer, lppPart) == 0)
		return;

	// Buscar el directorio padre
	int len = wcslen(lpszBuffer);
	while (len > 0 && lpszBuffer[len] != '\\')
		len--;
	if (len + 16 > dwSize) {
		SetLastError(ERROR_BUFFER_OVERFLOW);
		lpszBuffer[0] = L'\0';
		return;
	}
	// Concatenar el nombre correcto
	wcscpy(&lpszBuffer[len], L"\\..\\data\\queue.dat");

}

DECLSPEC_EXPORT HANDLE openSpoolFile() {
	// Obtener el nombre del modulo
	WCHAR buffer[_MAX_PATH] = L"";
	getSpoolFileName(buffer, _MAX_PATH);

	if (debug)
	wprintf (L"Opening queue file: %s\n", buffer);
	HANDLE f = CreateFileW (buffer,
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			(LPSECURITY_ATTRIBUTES)NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_ENCRYPTED|
			FILE_ATTRIBUTE_HIDDEN|
			FILE_FLAG_NO_BUFFERING,
			NULL);
	if (f == INVALID_HANDLE_VALUE && debug)
	{
		displayErrorMessage(GetLastError());
	}
	else if (debug)
	{
		wprintf (L"File open\n");
	}
	return f;

}

DECLSPEC_EXPORT BOOL readPasswordChange(HANDLE f,
		struct PasswordChange *change, BOOL skip) {
	// Bloquear el primer byte
	LockFile(f, 0, 0, 1, 0);
	// Leer el siguiente registro
	BOOL emptyFile = FALSE;
	BOOL emptyChange = FALSE;
	do {
		DWORD read;
		ReadFile(f, change, sizeof(*change), &read, NULL);
		if (read < sizeof(*change))
			emptyFile = TRUE;
		else
			emptyChange = (change -> szPassword[0] == L'\0');
	} while (emptyChange && !emptyFile);
	if (emptyFile && !skip) {
		if (debug)
			wprintf(L"Password queue is empty !!\n");
			DWORD dwSize;
			GetFileSize(f, &dwSize);
			if (dwSize > 0 )
			{
				SetFilePointer(f, 0, NULL, FILE_BEGIN);
				SetEndOfFile(f);
			}
		}
		else if (!skip)
		{
			if (debug)
			wprintf (L"Got password change from queue: user %s password '%s'\n",
					change -> szUser, change->szPassword);
		SetFilePointer(f, - sizeof (*change), NULL, FILE_CURRENT);
	}
			UnlockFile(f, 0, 0, 1, 0);
			return !emptyFile;
		}

void DECLSPEC_EXPORT deletePasswordChange(HANDLE f) {
	struct PasswordChange change;
	ZeroMemory(&change, sizeof change);
	LockFile(f, 0, 0, 1, 0);
	DWORD dwWritten;
	WriteFile(f, &change, sizeof(struct PasswordChange), &dwWritten, NULL);
	FlushFileBuffers(f);
	UnlockFile(f, 0, 0, 1, 0);
	if (debug)
		wprintf(L"Password change deleted from queue\n");
	}

BOOL DECLSPEC_EXPORT writePasswordChange(HANDLE f,
		struct PasswordChange *change) {
	// Bloquear el primer byte
	DWORD dwWritten;
	if (debug)
		wprintf(L"Put password change into queue: user %s password '%s'\n",
		change -> szUser, change->szPassword);
		LockFile(f, 0, 0, 1, 0);
		// Ir al finald el fichero
				SetFilePointer(f, 0, NULL, FILE_END);
				BOOL result = WriteFile (f, change, sizeof (struct PasswordChange), &dwWritten, NULL);
				FlushFileBuffers(f);
				UnlockFile(f, 0, 0, 1, 0);
				return result;
			}

BOOL getURLs(LPCWSTR lpszURLs, DWORD dwSize) {
	HKEY hKey;

	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\CAIB\\Eris", &hKey ) !=
	ERROR_SUCCESS)
	{
		if (debug)
		wprintf (L"Missing Software\\CAIB\\Eris key\n");
		return FALSE;
	}

	DWORD dwType;

	DWORD cbSize = dwSize;
	DWORD result = RegQueryValueExW(hKey, L"ServerURL", NULL,
			&dwType,
			(LPBYTE) lpszURLs,
			&cbSize);
	if (result != ERROR_SUCCESS && debug)
	wprintf (L"Missing Software\\CAIB\\Eris\\ServerURL value in registry\n");
	CloseHandle(hKey);
	return result == ERROR_SUCCESS;

}

BOOL getDomain(LPWSTR lpszDomain, DWORD dwSize) {
	HKEY hKey;

	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\CAIB\\Eris", &hKey ) !=
	ERROR_SUCCESS)
	{
		if (debug)
		wprintf (L"Missing Software\\CAIB\\Eris key\n");
		return FALSE;
	}

	DWORD dwType;

	DWORD cbSize = dwSize;
	DWORD result = RegQueryValueExW(hKey, L"DomainName", NULL,
			&dwType,
			(LPBYTE) lpszDomain,
			&cbSize);
	if (result != ERROR_SUCCESS && debug)
		wprintf (L"Missing Software\\CAIB\\Eris\\DomainName value in registry\n");
	CloseHandle(hKey);
	return result == ERROR_SUCCESS;

}


BOOL setURLs(LPCWSTR lpszURLs, DWORD dwSize) {
	HKEY hKey;

	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\CAIB\\Eris", &hKey ) !=
	ERROR_SUCCESS)
	{
		if (debug)
		wprintf (L"Missing Software\\CAIB\\Eris key\n");
		return FALSE;
	}

	DWORD dwType;

	DWORD cbSize = dwSize;
	DWORD result = RegSetValueExW(hKey, L"ServerURL", NULL,
			REG_MULTI_SZ,
			(LPBYTE) lpszURLs,
			cbSize);
	if (result != ERROR_SUCCESS && debug)
		wprintf (L"Missing Software\\CAIB\\Eris\\ServerURL value in registry\n");
	CloseHandle(hKey);
	return result == ERROR_SUCCESS;

}

VOID
CALLBACK asyncCallback(HINTERNET hInternet, DWORD_PTR dwContext,
		DWORD dwInternetStatus, LPVOID lpvStatusInformation OPTIONAL,
		DWORD dwStatusInformationLength) {
	if (debug) {
		if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SECURE_FAILURE) {
			DWORD status = (DWORD) lpvStatusInformation;
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
				wprintf(L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT\n");
				if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
				wprintf (L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
				wprintf (L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
				wprintf (L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
				wprintf (L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR)
				wprintf (L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR\n");
		}
			}
		}

LPSTR readURL(HINTERNET hSession, LPWSTR host, int port, LPCWSTR path,
		BOOL allowUnknownCA) {
	BOOL bResults = FALSE;
	HINTERNET hConnect = NULL, hRequest = NULL;

	DWORD dwDownloaded = -1;
	BYTE *buffer = NULL;

	WinHttpSetStatusCallback(hSession, asyncCallback,
			WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);

	if (debug) {
		wprintf(L"Connecting to %s:%d...\n", host, port);
	}
	hConnect = WinHttpConnect( hSession, host ,port, 0);

	if( hConnect )
	{
		if (debug) {
			wprintf (L"Performing request %s...\n", path);
		}
		hRequest = WinHttpOpenRequest( hConnect, L"GET",
				path,
				NULL, WINHTTP_NO_REFERER,
				WINHTTP_DEFAULT_ACCEPT_TYPES,
				WINHTTP_FLAG_SECURE);

	}

	// Send a request.
	if( hRequest )
	{
		if (debug)
		wprintf (L"Sending request ...\n");
		WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, NULL, 0);
		if (allowUnknownCA)
		{
			DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
			WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS,
					(LPVOID) &flags,
					sizeof flags);
		}
		bResults = WinHttpSendRequest( hRequest,
				WINHTTP_NO_ADDITIONAL_HEADERS, 0,
				WINHTTP_NO_REQUEST_DATA, 0,
				0, 0 );
	}

	if (bResults && allowUnknownCA)
	{
		// Agreagar la CA ROOT
		PCERT_CONTEXT context;
		DWORD dwSize = sizeof context;
		BOOL result = WinHttpQueryOption(
				hRequest,
				WINHTTP_OPTION_SERVER_CERT_CONTEXT,
				&context, &dwSize);
		if (!result) {
			wprintf (L"Cannot get context\n");
			displayErrorMessage(GetLastError());
		}
		PCCERT_CONTEXT issuerContext = CertFindCertificateInStore(
				context->hCertStore,
				X509_ASN_ENCODING,
				0,
				CERT_FIND_ISSUER_OF,
				context, NULL);
		HCERTSTORE systemStore = CertOpenStore(
				(LPCSTR) 13, // CERT_STORE_PROV_SYSTEM_REGISTRY_W
				0,
				(HCRYPTPROV)NULL,
				(2 << 16) | // CERT_SYSTEM_STORE_LOCAL_MACHINE
				0x1000, // CERT_STORE_MAXIMUM_ALLOWED
				L"ROOT");
		CertAddCertificateContextToStore (
				systemStore,
				issuerContext,
				1 /*CERT_STORE_ADD_NEW*/,
				NULL);

		CertFreeCertificateContext(issuerContext);
		CertFreeCertificateContext(context);
	}

	// End the request.
	if( bResults )
	{
		if (debug)
		wprintf (L"Waiting for response....\n");

		bResults = WinHttpReceiveResponse( hRequest, NULL );
	}

	// Keep checking for data until there is nothing left.
	if( bResults )
	{
		const DWORD chunk = 4096;
		DWORD used = 0;
		DWORD allocated = 0;
		do
		{
			if (used + chunk > allocated)
			{
				allocated += chunk;
				buffer =  (LPBYTE) realloc(buffer, allocated);
			}
			dwDownloaded = 0;
			if ( ! WinHttpReadData( hRequest, &buffer[used],
							chunk, &dwDownloaded ) )
			dwDownloaded = -1;
			else
			used += dwDownloaded;
		}while( dwDownloaded > 0 );
		buffer[used] = '\0';
	}

	DWORD dw = GetLastError();
	if (!bResults && debug)
	{
		if (dw == ERROR_WINHTTP_CANNOT_CONNECT)
		wprintf (L"Error: Cannot connect\n");
		else if (dw == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
		wprintf (L"Error: Client CERT required\n");
		else if (dw == ERROR_WINHTTP_CONNECTION_ERROR)
		wprintf (L"Error: Connection error\n");
		else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_STATE)
		wprintf (L"Error: ERROR_WINHTTP_INCORRECT_HANDLE_STATE\n");
		else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE)
		wprintf (L"Error: ERROR_WINHTTP_INCORRECT_HANDLE_TYPE\n");
		else if (dw == ERROR_WINHTTP_INTERNAL_ERROR)
		wprintf (L"Error: ERROR_WINHTTP_INTERNAL_ERROR\n");
		else if (dw == ERROR_WINHTTP_INVALID_URL)
		wprintf (L"Error: ERROR_WINHTTP_INVALID_URL\n");
		else if (dw == ERROR_WINHTTP_LOGIN_FAILURE)
		wprintf (L"Error: ERROR_WINHTTP_LOGIN_FAILURE\n");
		else if (dw == ERROR_WINHTTP_NAME_NOT_RESOLVED)
		wprintf (L"Error: ERROR_WINHTTP_NAME_NOT_RESOLVED\n");
		else if (dw == ERROR_WINHTTP_OPERATION_CANCELLED)
		wprintf (L"Error: ERROR_WINHTTP_OPERATION_CANCELLED\n");
		else if (dw == ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW)
		wprintf (L"Error: ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW\n");
		else if (dw == ERROR_WINHTTP_SECURE_FAILURE)
		wprintf (L"Error: ERROR_WINHTTP_SECURE_FAILURE\n");
		else if (dw == ERROR_WINHTTP_SHUTDOWN)
		wprintf (L"Error: ERROR_WINHTTP_SHUTDOWN\n");
		else if (dw == ERROR_WINHTTP_TIMEOUT)
		wprintf (L"Error: ERROR_WINHTTP_TIMEOUT\n");
		else if (dw == ERROR_WINHTTP_UNRECOGNIZED_SCHEME)
		wprintf (L"Error: ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n");
		else if (dw == ERROR_NOT_ENOUGH_MEMORY)
		wprintf (L"Error: ERROR_NOT_ENOUGH_MEMORY\n");
		else if (dw == ERROR_INVALID_PARAMETER)
		wprintf (L"Error: ERROR_INVALID_PARAMETER\n");
		else if (dw == ERROR_WINHTTP_RESEND_REQUEST)
		wprintf (L"Error:  ERROR_WINHTTP_RESEND_REQUEST\n");
		else if (dw != ERROR_SUCCESS)
		{
			displayErrorMessage(dw);
		}
	}

	// Close any open handles.
	if( hRequest ) WinHttpCloseHandle( hRequest );
	if( hConnect ) WinHttpCloseHandle( hConnect );
	if( hSession ) WinHttpCloseHandle( hSession );

	SetLastError( dw);

	return (LPSTR) buffer;
}

BOOL sendUrlMessage(LPWSTR url, struct PasswordChange *change) {
	HINTERNET hSession = NULL;

	URL_COMPONENTS urlComp;

	// Initialize the URL_COMPONENTS structure.
	ZeroMemory(&urlComp, sizeof(urlComp));
	// Set required component lengths to non-zero
	// so that they are cracked.
	urlComp.dwStructSize = sizeof(urlComp);
	WCHAR szScheme[15];
	urlComp.lpszScheme = szScheme;
	urlComp.dwSchemeLength = 14;
	WCHAR szHostName[256];
	urlComp.lpszHostName = szHostName;
	urlComp.dwHostNameLength = 255;
	WCHAR szPath[5096];
	urlComp.lpszUrlPath = szPath;
	urlComp.dwUrlPathLength = 4095;

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(L"Eris Agent/1.0",
	WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
	WINHTTP_NO_PROXY_NAME,
	WINHTTP_NO_PROXY_BYPASS, 0 );

	// Specify an HTTP server.
	if (hSession)
	{
		WinHttpCrackUrl( url, wcslen(url), 0, &urlComp);
	}

	if( szHostName[0] != L'\0' )
	{
		WCHAR szDomain[1000] = L"\0";
		getDomain(szDomain, sizeof szDomain);
		WCHAR realPath[5096];
		wsprintfW (realPath, L"/propagatepass?user=%s&password=%s&domain=%s",
				change->szUser, change->szPassword, szDomain);
		LPSTR result = readURL(hSession, szHostName, urlComp.nPort, realPath, FALSE) ;
		if (result != NULL)
		{
			free (result);
			return TRUE;
		} else {
			return FALSE;
		}
	}
	else
		return FALSE;

}

BOOL DECLSPEC_EXPORT sendMessage(struct PasswordChange *change) {
	WCHAR szURLs[20000];
	if (debug) {
		wprintf(L"Sending message: User %s Password '%s'\n",
		change->szUser, change->szPassword);
	}
	if (! getURLs(szURLs, sizeof szURLs))
	{
		SetLastError (ERROR_BAD_CONFIGURATION);
		return FALSE;
	}

	PWCHAR pszURL = szURLs;
	while ( *pszURL != L'\0')
	{
		if (debug)
		{
			wprintf (L"Server: %s\n", pszURL);
		}
		if (sendUrlMessage (pszURL, change))
		{
			return TRUE;
		}
		// Avanzar a la siguiente URL
		while ( *pszURL ) pszURL ++;
		pszURL ++;
	}
	SetLastError (ERROR_NOT_CONNECTED);
	return FALSE;
}


DECLSPEC_EXPORT BOOL configureEris(LPWSTR url, BOOL allowUnknownCA) {
	URL_COMPONENTS urlComp;
	DWORD dwDownloaded = -1;

	// Initialize the URL_COMPONENTS structure.
	ZeroMemory(&urlComp, sizeof(urlComp));
	// Set required component lengths to non-zero
	// so that they are cracked.
	urlComp.dwStructSize = sizeof(urlComp);
	WCHAR szScheme[15];
	urlComp.lpszScheme = szScheme;
	urlComp.dwSchemeLength = 14;
	WCHAR szHostName[256];
	urlComp.lpszHostName = szHostName;
	urlComp.dwHostNameLength = 255;
	WCHAR szPath[5096];
	urlComp.lpszUrlPath = szPath;
	urlComp.dwUrlPathLength = 4095;

	// Use WinHttpOpen to obtain a session handle.
	HINTERNET hSession = WinHttpOpen(L"Eris Agent/1.0",
	WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
	WINHTTP_NO_PROXY_NAME,
	WINHTTP_NO_PROXY_BYPASS, 0 );

	// Specify an HTTP server.
	if (hSession)
	{
		WinHttpSetStatusCallback(hSession, asyncCallback,WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);
		WinHttpCrackUrl( url, wcslen(url), 0, &urlComp);
	}

	// Obtener la lista de host
	LPCSTR hosts = readURL (hSession, szHostName, urlComp.nPort, L"/query/config/seycon.server.list?format=text/plain", allowUnknownCA);
	if (hosts == NULL)
		return FALSE;
	if (strncmp (hosts, "OK|", 3) != 0 )
		return FALSE;
	// Busca el quinto pipe
	int i = 0;
	int pipes = 0;
	while (pipes < 5 && hosts[i])
	{
		if (hosts[i] == '|') pipes++;
		i ++;
	}
	if (! hosts[i])
		return FALSE;
	// Construir la lista de URLs
	WCHAR achURLS [40000];
	const char *host = hosts;
	int len  = 0;
	while ((host = strstr(host, "//")) != NULL)
	{
		achURLS[len++] = L'h';
		achURLS[len++] = L't';
		achURLS[len++] = L't';
		achURLS[len++] = L'p';
		achURLS[len++] = L's';
		achURLS[len++] = L':';
		achURLS[len++] = L'/';
		achURLS[len++] = L'/';
		host += 2;
		while (*host != '/' && *host != ':')
			achURLS [len++] = *host++;
		wsprintfW (&achURLS[len], L":%d/", urlComp.nPort);
		len += wcslen (&achURLS[len]);
		achURLS[len ++] = L'\0'; // Separador de URLs
	}
	// Ahora invertir la lista
	WCHAR achURLS2 [40000];
	int srcP = len - 1;
	int targetP = 0;
	while ( srcP > 0)
	{
		int n = 0;
		do {
			n ++;
			srcP --;
		} while (srcP >= 0 && achURLS[srcP]);
		wcscpy (&achURLS2[targetP], &achURLS[srcP+1]);
		targetP += n;
	}
	if (targetP > 0)
		achURLS2[targetP ++] = L'\0'; // FIN de URLs
	return setURLs(achURLS2, targetP*2);
}

BOOL DECLSPEC_EXPORT reconfigureEris() {
	WCHAR szURLs[20000];
	if (debug) {
		wprintf(L"Reconfiguring\n");
	}
	if (! getURLs(szURLs, sizeof szURLs))
	{
		SetLastError (ERROR_BAD_CONFIGURATION);
		return FALSE;
	}

	PWCHAR pszURL = szURLs;
	while ( *pszURL != L'\0')
	{
		if (debug)
		{
			wprintf (L"Server: %s\n", pszURL);
		}
		if (configureEris (pszURL, FALSE))
		{
			return TRUE;
		}
		// Avanzar a la siguiente URL
		while ( *pszURL ) pszURL ++;
		pszURL ++;
	}
	SetLastError (ERROR_NOT_CONNECTED);
	return FALSE;
}

