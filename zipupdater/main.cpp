#include <iostream>
#include <filesystem>
#include <string>
#include <stdio.h>
#include <windows.h>
#include <zlib-ng.h>
#include "mz.h"
#include "mz_os.h"
#include "mz_strm.h"
#include "mz_strm_buf.h"
#include "mz_strm_split.h"
#include "mz_zip.h"
#include "mz_zip_rw.h"

#pragma comment(lib, "zlibstatic-ng.lib")

using namespace std;
namespace fs = std::filesystem;

bool extractZip(const std::string &zipFile, const std::string &outputDir) {
    void *reader = nullptr;
    int32_t err = MZ_OK;
    int32_t err_close = MZ_OK;

    std::cout << "Extracting archive: " << zipFile << std::endl;

    reader = mz_zip_reader_create();
    if (!reader) {
        std::cerr << "Failed to create ZIP reader." << std::endl;
        return false;
    }

    err = mz_zip_reader_open_file(reader, zipFile.c_str());
    if (err != MZ_OK) {
        std::cerr << "Error opening ZIP file: " << zipFile << " (error code: " << err << ")" << std::endl;
        mz_zip_reader_delete(&reader);
        return false;
    }

    err = mz_zip_reader_save_all(reader, outputDir.c_str());
    if (err == MZ_END_OF_LIST) {
        std::cout << "No files found in the archive." << std::endl;
        err = MZ_OK;
    } else if (err != MZ_OK) {
        std::cerr << "Error saving entries to disk: " << zipFile << " (error code: " << err << ")" << std::endl;
    }

    err_close = mz_zip_reader_close(reader);
    if (err_close != MZ_OK) {
        std::cerr << "Error closing ZIP reader (error code: " << err_close << ")" << std::endl;
        err = err_close;
    }

    mz_zip_reader_delete(&reader);

    return (err == MZ_OK);
}

bool copyDirectory(const fs::path& source, const fs::path& destination) {
    try {
        fs::create_directories(destination);
        for (const auto& entry : fs::recursive_directory_iterator(source)) {
            const auto& path = entry.path();
            auto relativePathStr = path.lexically_relative(source).string();
            fs::copy(path, destination / relativePathStr, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        }
    } catch (fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        MessageBox(nullptr, L"Usage: zipupdater.exe <zip_path> <extract_path> <target_path>", L"ZipUpdater", MB_ICONERROR);
        return 1;
    }

    std::string zipFilePath = argv[1];
    std::string extractPath = argv[2];
    std::string copyToPath = argv[3];

    if (!fs::exists(zipFilePath)) {
        MessageBox(nullptr, L"ZIP file does not exist!", L"ZipUpdater", MB_ICONERROR);
        return 1;
    }

    std::string currentPath = ".";
    if (!extractZip(zipFilePath, currentPath)) {
        MessageBox(nullptr, L"Failed to unzip the ZIP file.", L"ZipUpdater", MB_ICONERROR);
        return 1;
    }

    if (!copyDirectory(extractPath, copyToPath)) {
        MessageBox(nullptr, L"Failed to copy files to target path.", L"ZipUpdater", MB_ICONERROR);
        return 1;
    }

    MessageBox(nullptr, L"ZIP file extract and copy completed successfully!", L"ZipUpdater", MB_ICONINFORMATION);
    return 0;
}
