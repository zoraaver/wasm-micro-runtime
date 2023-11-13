#include "bh_read_file.h"

#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32) || defined(_WIN32_)
#include <io.h>
#else
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(_WIN32_)

#if defined(__MINGW32__) && !defined(_SH_DENYNO)
#define _SH_DENYNO 0x40
#endif

char *
bh_read_file_to_buffer(const char *filename, uint32 *ret_size)
{
    char *buffer;
    uint32 file_size, buf_size, read_size;

    if (!filename || !ret_size) {
        printf("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    wchar_t wfilename[PATH_MAX];
    if (!MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, PATH_MAX)) {
        printf("Failed to convert filename to wchar string, windows error code "
               "%u.\n",
               GetLastError());
        return NULL;
    }

    HANDLE handle =
        CreateFile2(wfilename, GENERIC_READ,
                    FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    OPEN_EXISTING, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("Failed to open file, error code: %u.\n", GetLastError());
        return NULL;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size))
        return NULL;

    file_size = (uint32)size.QuadPart;

    /* At lease alloc 1 byte to avoid malloc failed */
    buf_size = file_size > 0 ? file_size : 1;

    if (!(buffer = (char *)BH_MALLOC(buf_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        CloseHandle(handle);
        return NULL;
    }
#if WASM_ENABLE_MEMORY_TRACING != 0
    printf("Read file, total size: %u\n", file_size);
#endif
    DWORD read = 0;

    if (!ReadFile(handle, buffer, file_size, &read, NULL))
        return NULL;

    read_size = (uint32)read;

    CloseHandle(handle);

    if (read_size < file_size) {
        printf("Read file to buffer failed: read file content failed.\n");
        BH_FREE(buffer);
        return NULL;
    }

    *ret_size = file_size;
    return buffer;
}
#else /* else of defined(_WIN32) || defined(_WIN32_) */
char *
bh_read_file_to_buffer(const char *filename, uint32 *ret_size)
{
    char *buffer;
    int file;
    uint32 file_size, buf_size, read_size;
    struct stat stat_buf;

    if (!filename || !ret_size) {
        printf("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    if ((file = open(filename, O_RDONLY, 0)) == -1) {
        printf("Read file to buffer failed: open file %s failed.\n", filename);
        return NULL;
    }

    if (fstat(file, &stat_buf) != 0) {
        printf("Read file to buffer failed: fstat file %s failed.\n", filename);
        close(file);
        return NULL;
    }

    file_size = (uint32)stat_buf.st_size;

    /* At lease alloc 1 byte to avoid malloc failed */
    buf_size = file_size > 0 ? file_size : 1;

    if (!(buffer = BH_MALLOC(buf_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        close(file);
        return NULL;
    }
#if WASM_ENABLE_MEMORY_TRACING != 0
    printf("Read file, total size: %u\n", file_size);
#endif

    read_size = (uint32)read(file, buffer, file_size);
    close(file);

    if (read_size < file_size) {
        printf("Read file to buffer failed: read file content failed.\n");
        BH_FREE(buffer);
        return NULL;
    }

    *ret_size = file_size;
    return buffer;
}
#endif /* end of defined(_WIN32) || defined(_WIN32_) */
