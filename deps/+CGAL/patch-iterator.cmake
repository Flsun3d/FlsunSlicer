# ponytail: CGAL 5.4's Halfedge_around_{source,target,face}_iterator safe-bool operators
# call this->base(), a member the class never defines -- latent dead code that recent
# compilers (AppleClang 21) now instantiate and hard-error on. The real validity check is
# the graph pointer g (null only for a default-constructed iterator). Idempotent.
# Run in the extracted source dir (ExternalProject sets CWD = SOURCE_DIR for the patch step).
file(GLOB_RECURSE _headers "${CMAKE_SOURCE_DIR}/*.h")
foreach (_f IN LISTS _headers)
    if (_f MATCHES "/boost/graph/iterator\\.h$")
        file(READ "${_f}" _c)
        string(REPLACE "this->base() == nullptr" "this->g == nullptr" _new "${_c}")
        if (NOT _c STREQUAL _new)
            file(WRITE "${_f}" "${_new}")
            message(STATUS "patch-iterator: replaced base() validity check in ${_f}")
        endif ()
    endif ()
endforeach ()
