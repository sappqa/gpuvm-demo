#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>
#include <string>

#define GRANULARITY (1024 * 1024 * 2)
#define PAGE_SIZE (1024 * 1024 * 2)
#define ALIGNMENT (1024 * 1024 * 2)
#define VM_RANGE (1024 * 1024 * 2 * 64)
#define CU_CHECK(x) chkcu(x, #x)

void chkcu(CUresult result, const char* func) {
    if (result != CUDA_SUCCESS) {
        const char* perrorName;
        const char* perrorString;
        cuGetErrorName(result, &perrorName);
        cuGetErrorString(result, &perrorString);
        std::string errorMessage = "Error encountered: " + std::string(func) + " " + std::string(perrorName) + " " + std::string(perrorString);
        throw std::runtime_error(errorMessage);
    }
}

// nvcc gpuvm.cpp -o gpuvm -lcuda
int main(int argc, char** argv) {
    try {
        CUdeviceptr pvmRange;
        CUcontext ctx;
        CUdevice device;
        int driverVersion;

        cuInit(0);
        CU_CHECK(cuDeviceGet(&device, 0));
        CU_CHECK(cuCtxCreate(&ctx, 0, device));
        CU_CHECK(cuCtxSetCurrent(ctx));
        CU_CHECK(cuMemAddressReserve(&pvmRange, VM_RANGE, ALIGNMENT, 0, 0));
        std::cout << "Reserved virtual memory range of 2MiB from 0x" << std::hex << pvmRange << " to " << pvmRange + VM_RANGE << " this is a host virtual address space used to manage memory on the device" << std::endl;
        const unsigned int numpages = VM_RANGE / PAGE_SIZE;
        std::cout << "total num pages: " << std::dec << numpages << std::hex << std::endl;
        

        CUmemGenericAllocationHandle halloc;
        CUmemAllocationProp prop;
        prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
        prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
        // how do I optimize the size of this allocation? is there a specific size that performs better? how does the cuda driver handle this? does it pad allocations so they fit on boundaries? if so, what are the boundaries?
        // are these boundaries the same as the granularity, or is that different
        CU_CHECK(cuMemCreate(&halloc, PAGE_SIZE, &prop, 0));
        std::cout << "Reserving 1 page of uvm space at 0x" << halloc << " this is a uvm address." << std::endl;

        CUmemGenericAllocationHandle halloc2;
        CU_CHECK(cuMemCreate(&halloc2, PAGE_SIZE, &prop, 0));
        std::cout << "Reserving another page of uvm space at 0x" << halloc2 << " this is a uvm address." << std::endl;

        CU_CHECK(cuMemMap(pvmRange, PAGE_SIZE, 0, halloc, 0));
        std::cout << "Map1 success" << std::endl;

        CU_CHECK(cuMemMap(pvmRange + PAGE_SIZE, PAGE_SIZE, 0, halloc2, 0));
        std::cout << "Map2 success" << std::endl;

        CU_CHECK(cuMemUnmap(pvmRange, VM_RANGE));
        std::cout << "Unmap success" << std::endl;

        CU_CHECK(cuMemRelease(halloc));
        std::cout << "Freed page at 0x" <<  halloc << std::endl;

        CU_CHECK(cuMemRelease(halloc2));
        std::cout << "Freed page at 0x" <<  halloc2 << std::endl;

        CU_CHECK(cuMemAddressFree(pvmRange, VM_RANGE));
        std::cout << "Freed virtual memory range." << std::endl;
        CU_CHECK(cuCtxDestroy(ctx));
        return 0;
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

    // cuCtxGetCacheConfig();
    // cuCtxSetLimit();    
    // cuMemCreate
    // cuMemRelease

    // Calculates either the minimal or recommended granularity.
    // CUresult cuMemGetAllocationGranularity(size_t* granularity, const CUmemAllocationProp* prop,
    //                                        CUmemAllocationGranularity_flags option);

    // // Allocate an address range reservation.
    // CUresult cuMemAddressReserve(CUdeviceptr* ptr, size_t size, size_t alignment, CUdeviceptr addr,
    //                              unsigned long long flags);
    // // Free an address range reservation.
    // CUresult cuMemAddressFree(CUdeviceptr ptr, size_t size);

    // // Create a CUDA memory handle representing a memory allocation of a given size described by the given properties.
    // CUresult cuMemCreate(CUmemGenericAllocationHandle* handle, size_t size, const CUmemAllocationProp* prop,
    //                      unsigned long long flags);
    // // Release a memory handle representing a memory allocation which was previously allocated through cuMemCreate.
    // CUresult cuMemRelease(CUmemGenericAllocationHandle handle);
    // // Retrieve the contents of the property structure defining properties for this handle.
    // CUresult cuMemGetAllocationPropertiesFromHandle(CUmemAllocationProp* prop, CUmemGenericAllocationHandle handle);

    // // Maps an allocation handle to a reserved virtual address range.
    // CUresult cuMemMap(CUdeviceptr ptr, size_t size, size_t offset, CUmemGenericAllocationHandle handle,
    //                   unsigned long long flags);
    // // Unmap the backing memory of a given address range.
    // CUresult cuMemUnmap(CUdeviceptr ptr, size_t size);

    // // Get the access flags set for the given location and ptr.
    // CUresult cuMemGetAccess(unsigned long long* flags, const CUmemLocation* location, CUdeviceptr ptr);
    // // Set the access flags for each location specified in desc for the given virtual address range.
    // CUresult cuMemSetAccess(CUdeviceptr ptr, size_t size, const CUmemAccessDesc* desc, size_t count);