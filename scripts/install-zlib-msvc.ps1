$ZLIB_RELEASE = "1.3.1";
$ZLIB_RELEASE_ASSET = "zlib131.zip"
$ZLIB_DL_URL = "https://github.com/madler/zlib/releases/download/v${ZLIB_RELEASE}/${ZLIB_RELEASE_ASSET}"
$ZLIB_DL_SHA512 = "1f171880153b0120e1364baaf7d0a17f65086eff279f8f8c8538e5950097d1feee37cc173181676ba1e2aeb4565ba68749c814cd3e25bfb06271bea02feb7d94"
$ZLIB_DL_PATH = "$env:GITHUB_WORKSPACE\dl\${ZLIB_RELEASE_ASSET}"
$CurlArguments = '-s', '-Lf', '-o', "${ZLIB_DL_PATH}", "${ZLIB_DL_URL}"
& curl.exe @CurlArguments
$zlib_zip_hash = Get-FileHash -LiteralPath "${ZLIB_DL_PATH}" -Algorithm SHA512
if ($zlib_zip_hash.Hash -eq $ZLIB_DL_SHA512) {
    Write-Host "Successfully downloaded ${ZLIB_RELEASE_ASSET}"
}
Else {
    Write-Error "The downloaded ${ZLIB_RELEASE_ASSET} hash '$($zlib_zip_hash.Hash)' does not match the expected hash: '$ZLIB_DL_SHA512'"
}

Write-Host "Extracting zlib source..."
$ZLIB_SOURCE_PATH = "$env:GITHUB_WORKSPACE\buildtools\zlib-${ZLIB_RELEASE}"
Expand-Archive -LiteralPath "${ZLIB_DL_PATH}" -DestinationPath "$env:GITHUB_WORKSPACE\buildtools"

Write-Host "Building zlib source..."
$ZLIB_BUILD_PATH = "$env:GITHUB_WORKSPACE\buildtools\zlib_build"
cmake.exe -B "${ZLIB_BUILD_PATH}" -S "${ZLIB_SOURCE_PATH}"
cmake.exe --build "${ZLIB_BUILD_PATH}" --target zlibstatic
Copy-Item "${ZLIB_SOURCE_PATH}\zlib.h" "${ZLIB_BUILD_PATH}"

# Add CMAKE_DEFINES
Write-Output "CMAKE_DEFINES=-DZLIB_LIBRARY=${ZLIB_BUILD_PATH}\libzlibstatic.a -DZLIB_INCLUDE_DIR=${ZLIB_BUILD_PATH}" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append