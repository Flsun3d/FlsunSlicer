add_cmake_project(TIFF
    URL https://download.osgeo.org/libtiff/tiff-4.6.0.tar.gz  # ponytail: gitlab auto-archive hashes aren't byte-stable; osgeo release tarball is
    URL_HASH SHA256=88b3979e6d5c7e32b50d7ec72fb15af724f6ab2cbf7e10880c360a77e4b5d99a
    CMAKE_ARGS
        -Dtiff-tools:BOOL=OFF
        -Dtiff-tests:BOOL=OFF
        -Dlzma:BOOL=OFF
        -Dwebp:BOOL=OFF
        -Djbig:BOOL=OFF
        -Dzstd:BOOL=OFF
        -Dpixarlog:BOOL=OFF
        -Dlibdeflate:BOOL=OFF
        -Dlerc:BOOL=OFF  # ponytail: else TIFF's exported target pulls LERC::LERC, which breaks wxWidgets' find_package(TIFF)
)

set(DEP_TIFF_DEPENDS ZLIB PNG JPEG OpenGL)
