// WinInetRequestTest.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <Wininet.h>
#include <codecvt>
#include <assert.h>
#include <string>
#include <sstream>
#include <locale.h>
#include <conio.h>
#include "NhConversion.h"
#include <vector>
#include <iostream>
#include <fstream>

#pragma comment( lib, "Wininet.lib" )
using namespace std;
typedef std::basic_string<TCHAR>		tstring;
typedef std::basic_stringstream<TCHAR>	tstringstream;

#define URLBUFFER_SIZE		(4096)
#define	READBUFFER_SIZE		(4096)


string info(string strinfo);


bool HttpRequest(	tstring strUserAgent,
					tstring strUrl,
					bool bIsHttpVerbGet,
					tstring strParameter,
					tstring& rstrResult )
{
	// アウトプットの初期化
	rstrResult = tstring();

	// インプットのチェック
	if( 0 == strUrl.length() )
	{
		assert( !"URLが不正" );
		return false;
	}

	// 変数
	HINTERNET			hInternetOpen = NULL;
	HINTERNET			hInternetConnect = NULL;
	HINTERNET			hInternetRequest = NULL;
	char*				pszOptional = NULL;
	URL_COMPONENTS		urlcomponents;
	tstring				strServer;
	tstring				strObject;
	INTERNET_PORT		nPort;
	tstring				strVerb;
	tstring				strHeaders;
	tstringstream		ssRead;


	// URL解析
	ZeroMemory( &urlcomponents, sizeof(URL_COMPONENTS) );
	urlcomponents.dwStructSize = sizeof(URL_COMPONENTS);
	TCHAR szHostName[URLBUFFER_SIZE];
	TCHAR szUrlPath[URLBUFFER_SIZE];
	urlcomponents.lpszHostName	= szHostName;
	urlcomponents.lpszUrlPath	= szUrlPath;
	urlcomponents.dwHostNameLength	= URLBUFFER_SIZE;
	urlcomponents.dwUrlPathLength	= URLBUFFER_SIZE;
	if( !InternetCrackUrl(	strUrl.c_str(),
							(DWORD)strUrl.length(),
							0,
							&urlcomponents ) )
	{	// URLの解析に失敗
		assert( !"URL解析に失敗" );
		return false;
	}
	strServer = urlcomponents.lpszHostName;
	strObject = urlcomponents.lpszUrlPath;
	nPort	  = urlcomponents.nPort;

	// HTTPかHTTPSかそれ以外か
	DWORD dwFlags = 0;
	if(		 INTERNET_SCHEME_HTTP == urlcomponents.nScheme )
	{	// HTTP
		dwFlags = INTERNET_FLAG_RELOAD				// 要求されたファイル、オブジェクト、またはフォルダ一覧を、キャッシュからではなく、元のサーバーから強制的にダウンロードします。
				| INTERNET_FLAG_DONT_CACHE			// 返されたエンティティをキャシュへ追加しません。
				| INTERNET_FLAG_NO_AUTO_REDIRECT;	// HTTP だけで使用され、リダイレクトが HttpSendRequest で処理されないことを指定します。
	}
	else if( INTERNET_SCHEME_HTTPS == urlcomponents.nScheme )
	{	// HTTPS
		dwFlags = INTERNET_FLAG_RELOAD				// 要求されたファイル、オブジェクト、またはフォルダ一覧を、キャッシュからではなく、元のサーバーから強制的にダウンロードします。
				| INTERNET_FLAG_DONT_CACHE			// 返されたエンティティをキャシュへ追加しません。
				| INTERNET_FLAG_NO_AUTO_REDIRECT	// HTTP だけで使用され、リダイレクトが HttpSendRequest で処理されないことを指定します。
				| INTERNET_FLAG_SECURE						// 安全なトランザクションを使用します。これにより、SSL/PCT を使うように変換され、HTTP 要求だけで有効です。 
				| INTERNET_FLAG_IGNORE_CERT_DATE_INVALID	// INTERNET_FLAG_IGNORE_CERT_DATE_INVALID、INTERNET_FLAG_IGNORE_CERT_CN_INVALID
				| INTERNET_FLAG_IGNORE_CERT_CN_INVALID;		// は、証明書に関する警告を無視するフラグ
	}
	else
	{
		assert( !"HTTPでもHTTPSでもない" );
		return false;
	}

	// GETかPOSTか
	if( bIsHttpVerbGet )
	{	// GET
		strVerb		= _T("GET");
		strHeaders	= _T("token: c733f1af-7c36-4ee2-b97f-6b02fa51f0da");
		if( 0 != strParameter.length() )
		{	// オブジェクトとパラメータを「?」で連結
			strObject += _T("?") + strParameter;
		}
	}
	else
	{	// POST
		strVerb		= _T("POST");
		strHeaders	= _T("Content-Type: application/x-www-form-urlencoded");
		if( 0 != strParameter.length() )
		{	// パラメータを、送信するオプションデータに変換する
			pszOptional = NhT2M( strParameter.c_str() );	// char文字列に変換
		}
	}

	// WinInetの初期化
	hInternetOpen = InternetOpen(	strUserAgent.c_str(),
									INTERNET_OPEN_TYPE_PRECONFIG,
									NULL, NULL, 0 );
	if( NULL == hInternetOpen )
	{
		assert( !"WinInetの初期化に失敗" );
		goto LABEL_ERROR;
	}

	// HTTP接続
	hInternetConnect = InternetConnect(	hInternetOpen,
										strServer.c_str(),
										nPort,
										NULL,
										NULL,
										INTERNET_SERVICE_HTTP,
										0,
										0 );
	if( NULL == hInternetConnect )
	{
		assert( !"HTTP接続に失敗" );
		goto LABEL_ERROR;
	}

	// HTTP接続を開く
	hInternetRequest = HttpOpenRequest( hInternetConnect,
										strVerb.c_str(),
										strObject.c_str(),
										NULL,
										NULL,
										NULL,
										dwFlags,
										NULL );
	if( NULL == hInternetRequest )
	{
		assert( !"HTTP接続を開くに失敗" );
		goto LABEL_ERROR;
	}

	// HTTP要求送信
	if( !HttpSendRequest(	hInternetRequest,
							strHeaders.c_str(),
							(DWORD)strHeaders.length(),
							(LPVOID)((char*)pszOptional),
							pszOptional ? (DWORD)(strlen(pszOptional) * sizeof(char)) : 0 ) )
	{
		assert( !"HTTP要求送信に失敗" );
		goto LABEL_ERROR;
	}

	// HTTP要求に対応するステータスコードの取得
	DWORD dwStatusCode;
	DWORD dwLength = sizeof(DWORD);
	if( !HttpQueryInfo(	hInternetRequest,
						HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
						&dwStatusCode,
						&dwLength,
						0 ) )
	{
		assert( !"HTTP要求に対応するステータスコードの取得に失敗" );
		goto LABEL_ERROR;
	}
	if( HTTP_STATUS_OK != dwStatusCode )
	{
		assert( !"ステータスコードがOKでない" );
		goto LABEL_ERROR;
	}

	// HTTPファイル読み込み
	char szReadBuffer[READBUFFER_SIZE + 1];
	while( 1 )
	{
		DWORD dwRead = 0;
		if( !InternetReadFile( hInternetRequest, szReadBuffer, READBUFFER_SIZE, &dwRead ) )
		{
			assert( !"HTTPファイル読み込みに失敗" );
			goto LABEL_ERROR;
		}
		if( 0 == dwRead )
		{
			break;
		}
		szReadBuffer[dwRead] = '\0';	// 終端文字「\0」の付加
		size_t length = dwRead + 1;
		LPWSTR	pszWideChar = (LPWSTR)malloc( length * sizeof(WCHAR) );
		MultiByteToWideChar(	CP_UTF8,	// CODE PAGE: UTF-8
								0,
								szReadBuffer,
								-1,
								pszWideChar,
								(int)length );	// UTF-8文字列をANSI文字列に変換
		TCHAR* pszTchar = NhW2T( pszWideChar );	// WideChar文字列をTCHAR文字列に変換
		ssRead << pszTchar;	// ストリーム文字列に流し込む
		free( pszTchar );
		free( pszWideChar );
	}

	// ストリーム文字列を、出力文字列に変換
	rstrResult = ssRead.str().c_str();
	

	if( pszOptional ){ free( pszOptional ); }
	InternetCloseHandle(hInternetRequest);
	InternetCloseHandle(hInternetConnect);
	InternetCloseHandle(hInternetOpen);
	return true;

LABEL_ERROR:
	if( pszOptional ){ free( pszOptional ); }
	InternetCloseHandle(hInternetRequest);
	InternetCloseHandle(hInternetConnect);
	InternetCloseHandle(hInternetOpen);

	return false;
}

std::vector<int> find_all(const std::string str, const std::string subStr) {
	std::vector<int> result;

	int subStrSize = subStr.size();
	int pos = str.find(subStr);

	while (pos != std::string::npos) {
		result.push_back(pos);
		pos = str.find(subStr, pos + subStrSize);
	}

	return result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	tstring strUserAgent	= _T("Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.99 Safari/537.36");
	tstring strUrl			= _T("http://otogi-api.trafficmanager.net/api/UFriend/Detail/");
	bool bIsHttpVerbGet		= true;
	tstring strParameter	= _T(" ");
	tstring strUrl2 = strUrl;
	fstream config;
	fstream data;
	fstream data2;
	int num = 0;
	config.open("config.txt", ios::out);
	data.open("data.txt", ios::app);
	data2.open("data2.txt", ios::in);
	
	string str2;
	//data2 >> str2;

	//str2 = info(str2);
	
	//cout << str2 << endl;
	



	//exit(0);

	if (!config.is_open()) {
		return EXIT_FAILURE;
	}

	if (!data.is_open()) {
		return EXIT_FAILURE;
	}
	cout << "No = " << endl;
	cin >> num;

	int max = 1;
	cout << "Max = " << endl;
	cin >> max;

	if (num == 0) {
		cout << "num=0です" << endl;
		exit(0);
	}
	
	cout << "ID = "<<num  << "Max = "<<max<<endl;
	_getch();

//処理
	int y = 0;
	for (int i = num; i <  max+1; i++) {
		
		strUrl = strUrl2;
		strUrl += std::to_wstring(i);


		_tprintf(_T("%s"), strUrl.c_str());


		tstring strResult;
		if (!HttpRequest(strUserAgent,
			strUrl,
			bIsHttpVerbGet,
			strParameter,
			strResult))
		{
			return false;
		}

	//	setlocale(LC_ALL, "Japanese");
	//	_tprintf(_T("%s"), strResult.c_str());

		wstring ws(strResult);
		string str(ws.begin(), ws.end());
		
		using codecvt_wchar = std::codecvt<wchar_t, char, std::mbstate_t>;
		
		std::locale loc("");
		std::wstring_convert<codecvt_wchar> cv(
		&std::use_facet<codecvt_wchar>(loc));
		
		//string name = cv.to_bytes(ws);
		
		
		Sleep(300);
		cout << endl;
		
		
		string output;
		stringstream ss;

		output =info(str);
		if (output == "NULL"){
			
			ss << "\"Id\":" << i << "," << "\"Level\"" << ":0," << "\"LastLogin\"" << ":" << "\"NULL\"" ;
			output = ss.str();
		}
			data << output << endl;

		y = i;
		if (GetAsyncKeyState('p')) {
			cout << "終了" << endl;
			break;
		}
		

	}
	config << y << endl;
	config.close();
	data.close();
	cout << "end" << endl;
	while (1){
		if (GetAsyncKeyState('p')) {
			cout << "終了" << endl;
			break;
		}
	}
	return 0;
}

string info(string strinfo) {

	string str = strinfo;
	string rtn;
	string subStr1 = "\"";
	string subStr2 = ",";
	string strId;
	string strName;
	string strLv;
	string strLogin;
	int num[500]={};
	int num2[500] = {};
	int i = 0;

	if (str.size() < 50){
		return "NULL";
	}

	str.erase(str.find("\"Name\"") - 1, str.find("\"Level\"") - str.find("\"Name\""));

	std::vector<int> findVecStart = find_all(str, subStr1);
	std::vector<int> findVecEnd = find_all(str, subStr2);

	for (const auto &pos : findVecStart) {
		num[i] = pos; //std::cout << pos << "番目\n";
		i++;
	}
	i = 0;
	for (const auto &pos2 : findVecEnd) {
		num2[i] = pos2;// std::cout << pos2 << "番目\n";
		i++;
	}

	

	strId = str.substr(num[0], num2[0]-1);
	//strName = str.substr(num2[1]+1, num2[2]-num2[1]-1);
	strLv = str.substr(num2[1]+1, num2[2] - num2[1]-1);
	strLogin = str.substr(num2[4] + 1, str.find("\"FriendStatus\"") - num2[4] -2);
	//rtn = strId+"," + strLv+","+strName+","+strLogin;
	rtn = strId + "," + strLv + "," + strLogin;
	return rtn;
}

