//#include <new>
//
//unsigned long long N_ALLOCATED_BYTES[4096] = { 0 };
//
//void* operator new(std::size_t nBytes) {
//	void* p = std::malloc(nBytes + sizeof(size_t));
//	*(size_t*)p = nBytes;
//	if (p == nullptr) throw std::bad_alloc();
//	if (nBytes/8 < 4096)
//		N_ALLOCATED_BYTES[nBytes/8] += nBytes;
//	return (char*)p + sizeof(size_t);
//}
//
//void operator delete(void* p) {
//	size_t size = ((size_t*)p)[-1];
//	if (size/8 < 4096)
//	N_ALLOCATED_BYTES[size/8] -= size;
//	std::free((size_t*)p - 1);
//
//}