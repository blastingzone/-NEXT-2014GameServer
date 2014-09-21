#include "stdafx.h"
#include "Crypt.h"

Crypt::Crypt()
{

}

Crypt::~Crypt()
{

}

void Crypt::CreatePrivateKey()
{
	//provider handle을 획득
	if (!CryptAcquireContext(
		&mProvParty,
		NULL,
		MS_ENH_DSS_DH_PROV,
		PROV_DSS_DH,
		CRYPT_VERIFYCONTEXT))
		Release();

	// 임시 private key를 생성
	if (!CryptGenKey(
		mProvParty,
		CALG_DH_EPHEM,
		DHKEYSIZE << 16 | CRYPT_EXPORTABLE | CRYPT_PREGEN,
		&mProvParty))
		Release();

	// private key에 대한 설정
	if (!CryptSetKeyParam(
		mProvParty,
		KP_P,
		(PBYTE)&P,
		0))
		Release();

	// private key에 대한 generator설정
	if (!CryptSetKeyParam(
		mProvParty,
		KP_G,
		(PBYTE)&G,
		0))
		Release();

	// private key에 대한 비밀값 생성
	if (!CryptSetKeyParam(
		mProvParty,
		KP_X,
		NULL,
		0))
		Release();
}

void Crypt::ExportPublicKey()
{
	// Public key value, (G^X) mod P is calculated.
	DWORD dwDataLen;

	// key BLOB의 크기를 받아옴
	if (!CryptExportKey(
		mPrivateKey,
		NULL,
		PUBLICKEYBLOB,
		0,
		NULL,
		&dwDataLen))
		Release();

	// key BLOB를 위한 메모리 할당
	if (!(mKeyBlob = (PBYTE)malloc(dwDataLen)))
		Release();

	// key BLOB를 얻음
	if(!CryptExportKey(
		mPrivateKey,
		0,
		PUBLICKEYBLOB,
		0,
		mKeyBlob,
		&dwDataLen))
		Release();
}

void Crypt::ImportPublicKey()
{
	if (!CryptImportKey(
		mProvParty,
		pbKeyBlob2,
		dwDataLen2,
		mPrivateKey,
		0,
		&hSessionKey2))
		Release();
}

void Crypt::ConvertRC4()
{
	ALG_ID Algid = CALG_RC4;

	// Enable the party 1 public session key for use by setting the 
	// ALGID.
	if (!CryptSetKeyParam(
		hSessionKey1,
		KP_ALGID,
		(PBYTE)&Algid,
		0))
		Release();

}

bool Crypt::RC4Encyrpt()
{
	// Get the size.
	DWORD dwLength = sizeof(g_rgbData);
	if(!CryptEncrypt(
		hSessionKey1,
		0,
		TRUE,
		0,
		NULL,
		&dwLength,
		sizeof(g_rgbData)))
	Release();

	// 암호화될 데이터를 위한 버퍼 할당
	pbData = (PBYTE)malloc(dwLength);
	if (!pbData)
		Release();

	memcpy(pbData, g_rgbData, sizeof(g_rgbData));

	// 데이터를 암호화
	dwLength = sizeof(g_rgbData);
	if (!CryptEncrypt(
		hSessionKey1,
		0,
		TRUE,
		0,
		pbData,
		&dwLength,
		sizeof(g_rgbData)))
		Release();
}

bool Crypt::RC4Decrypt()
{
	dwLength = sizeof(g_rgbData);
	if (!CryptDecrypt(
		hSessionKey2,
		0,
		TRUE,
		0,
		pbData,
		&dwLength))
		Release();
}



void Crypt::Release()
{
	if (mSessionKey)
	{
		CryptDestroyKey(mSessionKey);
		mSessionKey = NULL;
	}

	if (mKeyBlob)
	{
		free(mKeyBlob);
		mKeyBlob = NULL;
	}

	if (mPrivateKey)
	{
		CryptDestroyKey(mPrivateKey);
		mPrivateKey = NULL;
	}

	if (mProvParty)
	{
		CryptReleaseContext(mProvParty, 0);
		mProvParty = NULL;
	}
}







