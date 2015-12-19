#include <windows.h>
#include <wincrypt.h>

int main()
{
	HCRYPTPROV provider;
	CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptGenRandom(provider, len, buf);
	CryptReleaseContext(provider, 0);
}
