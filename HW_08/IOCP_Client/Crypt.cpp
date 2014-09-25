#include "stdafx.h"
#include "Crypt.h"
#include "Exception.h"

Crypt::Crypt()
{
	mP.cbData = DHKEYSIZE / 8;
	mP.pbData = (BYTE*)(GRgbPrime);

	mG.cbData = DHKEYSIZE / 8;
	mG.pbData = (BYTE*)(GRgbGenerator);
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
	{ 
		Release();
		CRASH_ASSERT(false);
	}
		

	// 임시 private key를 생성
	if (!CryptGenKey(
		mProvParty,
		CALG_DH_EPHEM,
		DHKEYSIZE << 16 | CRYPT_EXPORTABLE | CRYPT_PREGEN,
		&mPrivateKey))
	{
		Release();
		CRASH_ASSERT(false);
	}

	// private key에 대한 설정
	if (!CryptSetKeyParam(
		mPrivateKey,
		KP_P,
		(PBYTE)&mP,
		0))
	{
		Release();
		CRASH_ASSERT(false);
	}

	// private key에 대한 generator설정
	if (!CryptSetKeyParam(
		mPrivateKey,
		KP_G,
		(PBYTE)&mG,
		0))
	{
		Release();
		CRASH_ASSERT(false);
	}

	// private key에 대한 비밀값 생성
	if (!CryptSetKeyParam(
		mPrivateKey,
		KP_X,
		NULL,
		0))
	{
		Release();
		CRASH_ASSERT(false);
	}
}

void Crypt::ExportPublicKey()
{
	// key BLOB의 크기를 받아옴
	if (!CryptExportKey(
		mPrivateKey,
		NULL,
		PUBLICKEYBLOB,
		0,
		NULL,
		&mKeyBlobLen))
	{
		Release();
		CRASH_ASSERT(false);
	}

	// key BLOB를 위한 메모리 할당
	if (!(mKeyBlob = (PBYTE)malloc(mKeyBlobLen)))
	{
		Release();
		CRASH_ASSERT(false);
	}

	// key BLOB를 얻음
	if(!CryptExportKey(
		mPrivateKey,
		0,
		PUBLICKEYBLOB,
		0,
		mKeyBlob,
		&mKeyBlobLen))
	{
		Release();
		CRASH_ASSERT(false);
	}
}

void Crypt::ImportPublicKey(PBYTE remoteKeyBlob, DWORD remoteDataLen)
{
	if (!CryptImportKey(
		mProvParty,
		remoteKeyBlob,
		remoteDataLen,
		mPrivateKey,
		0,
		&mSessionKey))
	{
		Release();
		CRASH_ASSERT(false);
	}
}

void Crypt::ConvertRC4()
{
	ALG_ID Algid = CALG_RC4;

	// Enable the party 1 public session key for use by setting the 
	// ALGID.
	if (!CryptSetKeyParam(
		mSessionKey,
		KP_ALGID,
		(PBYTE)&Algid,
		0))
	{
		Release();
		CRASH_ASSERT(false);
	}

}

void Crypt::RC4Encyrpt(PBYTE data, DWORD length)
{
	// Get the size.
	DWORD dwLength = length;
	if(!CryptEncrypt(
		mSessionKey,
		0,
		TRUE,
		0,
		data,
		&dwLength,
		length))
	{
		Release();
		CRASH_ASSERT(false);
	}
}

void Crypt::RC4Decrypt(PBYTE data, DWORD length)
{
	DWORD dwLength = length;
	if (!CryptDecrypt(
		mSessionKey,
		0,
		TRUE,
		0,
		data,
		&dwLength))
	{
		Release();
		CRASH_ASSERT(false);
	}
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







