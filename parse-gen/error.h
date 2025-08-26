#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

    // FIXME: More descriptive errors
    enum glcpg_error
    {
        E_GLCPG_INVALIDINPUT = -127,
        E_GLCPG_MEMORYERROR,
        E_GLCPG_IOERROR,
        E_GLCPG_NOTENOUGHSIZE,
        E_GLCPG_UNEXPECTED,
    };

#if defined(__cplusplus)
}
#endif