#include <tchar.h>
#include <windows.h>
#include <wincrypt.h>

// The key size, in bits.
#define DHKEYSIZE 512

// Prime in little-endian format.
static const BYTE g_rgbPrime[] =
{
	0x91, 0x02, 0xc8, 0x31, 0xee, 0x36, 0x07, 0xec,
	0xc2, 0x24, 0x37, 0xf8, 0xfb, 0x3d, 0x69, 0x49,
	0xac, 0x7a, 0xab, 0x32, 0xac, 0xad, 0xe9, 0xc2,
	0xaf, 0x0e, 0x21, 0xb7, 0xc5, 0x2f, 0x76, 0xd0,
	0xe5, 0x82, 0x78, 0x0d, 0x4f, 0x32, 0xb8, 0xcb,
	0xf7, 0x0c, 0x8d, 0xfb, 0x3a, 0xd8, 0xc0, 0xea,
	0xcb, 0x69, 0x68, 0xb0, 0x9b, 0x75, 0x25, 0x3d,
	0xaa, 0x76, 0x22, 0x49, 0x94, 0xa4, 0xf2, 0x8d
};

// Generator in little-endian format.
static BYTE g_rgbGenerator[] =
{
	0x02, 0x88, 0xd7, 0xe6, 0x53, 0xaf, 0x72, 0xc5,
	0x8c, 0x08, 0x4b, 0x46, 0x6f, 0x9f, 0x2e, 0xc4,
	0x9c, 0x5c, 0x92, 0x21, 0x95, 0xb7, 0xe5, 0x58,
	0xbf, 0xba, 0x24, 0xfa, 0xe5, 0x9d, 0xcb, 0x71,
	0x2e, 0x2c, 0xce, 0x99, 0xf3, 0x10, 0xff, 0x3b,
	0xcb, 0xef, 0x6c, 0x95, 0x22, 0x55, 0x9d, 0x29,
	0x00, 0xb5, 0x4c, 0x5b, 0xa5, 0x63, 0x31, 0x41,
	0x13, 0x0a, 0xea, 0x39, 0x78, 0x02, 0x6d, 0x62
};

BYTE g_rgbData[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

int _tmain(int argc, _TCHAR* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	BOOL fReturn;
	HCRYPTPROV hProvParty1 = NULL;
	HCRYPTPROV hProvParty2 = NULL;
	DATA_BLOB P;
	DATA_BLOB G;
	HCRYPTKEY hPrivateKey1 = NULL;
	HCRYPTKEY hPrivateKey2 = NULL;
	PBYTE pbKeyBlob1 = NULL;
	PBYTE pbKeyBlob2 = NULL;
	HCRYPTKEY hSessionKey1 = NULL;
	HCRYPTKEY hSessionKey2 = NULL;
	PBYTE pbData = NULL;

	/************************
	Construct data BLOBs for the prime and generator. The P and G
	values, represented by the g_rgbPrime and g_rgbGenerator arrays
	respectively, are shared values that have been agreed to by both
	parties.
	************************/
	P.cbData = DHKEYSIZE / 8;
	P.pbData = (BYTE*)(g_rgbPrime);

	G.cbData = DHKEYSIZE / 8;
	G.pbData = (BYTE*)(g_rgbGenerator);







ErrorExit:


	return 0;
}



class Crypt
{
public:
	Crypt();
	~Crypt();

	void CreatePrivateKey();
	void ExportPublicKey();
	void ImportPublicKey();
	void ConvertRC4();

	bool RC4Encyrpt();
	bool RC4Decrypt();

	void Release();

private:

	DATA_BLOB mP;
	DATA_BLOB mG;

	HCRYPTPROV	mProvParty = NULL;
	HCRYPTKEY	mPrivateKey = NULL;
	PBYTE		mKeyBlob = NULL;
	HCRYPTKEY	mSessionKey = NULL;

	//PBYTE pbData = NULL;
};