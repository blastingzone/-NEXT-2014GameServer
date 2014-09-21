#include "stdafx.h"
#include "Crypt.h"

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
	//provider handle�� ȹ��
	if (!CryptAcquireContext(
		&mProvParty,
		NULL,
		MS_ENH_DSS_DH_PROV,
		PROV_DSS_DH,
		CRYPT_VERIFYCONTEXT))
		Release();

	// �ӽ� private key�� ����
	if (!CryptGenKey(
		mProvParty,
		CALG_DH_EPHEM,
		DHKEYSIZE << 16 | CRYPT_EXPORTABLE | CRYPT_PREGEN,
		&mProvParty))
		Release();

	// private key�� ���� ����
	if (!CryptSetKeyParam(
		mProvParty,
		KP_P,
		(PBYTE)&mP,
		0))
		Release();

	// private key�� ���� generator����
	if (!CryptSetKeyParam(
		mProvParty,
		KP_G,
		(PBYTE)&mG,
		0))
		Release();

	// private key�� ���� ��а� ����
	if (!CryptSetKeyParam(
		mProvParty,
		KP_X,
		NULL,
		0))
		Release();
}

void Crypt::ExportPublicKey()
{
	// key BLOB�� ũ�⸦ �޾ƿ�
	if (!CryptExportKey(
		mPrivateKey,
		NULL,
		PUBLICKEYBLOB,
		0,
		NULL,
		&mDataLen))
		Release();

	// key BLOB�� ���� �޸� �Ҵ�
	if (!(mKeyBlob = (PBYTE)malloc(mDataLen)))
		Release();

	// key BLOB�� ����
	if(!CryptExportKey(
		mPrivateKey,
		0,
		PUBLICKEYBLOB,
		0,
		mKeyBlob,
		&mDataLen))
		Release();
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
		Release();
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
		Release();

}

bool Crypt::RC4Encyrpt(PBYTE data, DWORD length)
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
	Release();
}

bool Crypt::RC4Decrypt(PBYTE data, DWORD length)
{
	DWORD dwLength = length;
	if (!CryptDecrypt(
		mSessionKey,
		0,
		TRUE,
		0,
		data,
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







