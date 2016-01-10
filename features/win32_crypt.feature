#include <windows.h>
#include <wincrypt.h>

int main()
{
	HCRYPTPROV provider;
	size_t len;
	char buf[1];
	CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptGenRandom(provider, len, buf);
	CryptReleaseContext(provider, 0);
}
