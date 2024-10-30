#!/bin/bash

if [ ! -d "vcpkg" ]; then
    git clone --depth 1 https://github.com/microsoft/vcpkg
    ./vcpkg/bootstrap-vcpkg.sh
fi

./vcpkg/vcpkg install --overlay-triplets=custom-triplets --triplet=x64-osx-1013 zlib libogg opus opusfile libvorbis libmad libflac libxmp
./vcpkg/vcpkg install --overlay-triplets=custom-triplets --triplet=arm64-osx-11 zlib libogg opus opusfile libvorbis libmad libflac libxmp

mkdir -p libs_universal
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libogg.a ./vcpkg/installed/arm64-osx-11/lib/libogg.a -output ./libs_universal/libogg.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libopus.a ./vcpkg/installed/arm64-osx-11/lib/libopus.a -output ./libs_universal/libopus.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libopusfile.a ./vcpkg/installed/arm64-osx-11/lib/libopusfile.a -output ./libs_universal/libopusfile.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libvorbis.a ./vcpkg/installed/arm64-osx-11/lib/libvorbis.a -output ./libs_universal/libvorbis.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libvorbisenc.a ./vcpkg/installed/arm64-osx-11/lib/libvorbisenc.a -output ./libs_universal/libvorbisenc.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libvorbisfile.a ./vcpkg/installed/arm64-osx-11/lib/libvorbisfile.a -output ./libs_universal/libvorbisfile.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libz.a ./vcpkg/installed/arm64-osx-11/lib/libz.a -output ./libs_universal/libz.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libmad.a ./vcpkg/installed/arm64-osx-11/lib/libmad.a -output ./libs_universal/libmad.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libFLAC.a ./vcpkg/installed/arm64-osx-11/lib/libFLAC.a -output ./libs_universal/libFLAC.a
lipo -create ./vcpkg/installed/x64-osx-1013/lib/libxmp.a ./vcpkg/installed/arm64-osx-11/lib/libxmp.a -output ./libs_universal/libxmp.a