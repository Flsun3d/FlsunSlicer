# ponytail: libpng 1.6.35 pngpriv.h includes <fp.h> (a classic-Mac header removed from
# modern macOS SDKs) whenever TARGET_OS_MAC is defined -- which it always is on Apple now.
# Force the <math.h> branch so it builds under recent Xcode. Idempotent: no-op if absent.
# Run in the extracted source dir (ExternalProject sets CWD = SOURCE_DIR for the patch step).
file(GLOB_RECURSE _headers "${CMAKE_SOURCE_DIR}/*.h")
foreach (_f IN LISTS _headers)
    if (_f MATCHES "/pngpriv\\.h$")
        file(READ "${_f}" _c)
        string(REPLACE "<fp.h>" "<math.h>" _new "${_c}")
        if (NOT _c STREQUAL _new)
            file(WRITE "${_f}" "${_new}")
            message(STATUS "patch-pngpriv: replaced <fp.h> with <math.h> in ${_f}")
        endif ()
    endif ()
endforeach ()
