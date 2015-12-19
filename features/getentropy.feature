#include <unistd.h>

int main()
{
	int(*p)(void*, size_t) = getentropy;
	return getentropy((void*)(char[]{1}, (size_t)0);
}
