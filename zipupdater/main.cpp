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

    mz_zip_reader_set_encoding(reader, MZ_ENCODING_UTF8);

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
    // Set DPI awareness for high-DPI displays
    SetProcessDPIAware();

    // Check for the correct number of arguments
    if ((argc < 4) || (argc > 5)) {
        MessageBox(nullptr, L"Usage: zipupdater.exe <zip_path> <copyfrom_path> <copyto_path> [--chinese]", L"ZipUpdater", MB_ICONWARNING);
        return 1;
    }

    // Set the working directory to the executable's directory
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH); // Get the full path of the executable
    std::wstring exeDir = fs::path(exePath).parent_path().wstring(); // Get the directory of the executable
    SetCurrentDirectory(exeDir.c_str()); // Set it as the current working directory

    // Parse arguments
    std::string zipFilePath = argv[1];
    std::string copyFromPath = argv[2];
    std::string copyToPath = argv[3];
    bool useChinese = (argc == 5 && std::string(argv[4]) == "--chinese");

    // Check if the ZIP file exists
    if (!fs::exists(zipFilePath)) {
        MessageBox(nullptr,
                   useChinese ? L"ZIP 文件不存在！" : L"The ZIP file does not exist!",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Extract the ZIP file
    std::string currentPath = ".";
    if (!extractZip(zipFilePath, currentPath)) {
        MessageBox(nullptr,
                   useChinese ? L"解压 ZIP 文件时发生错误！" : L"An error occurred while extracting the ZIP file!",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Copy the files
    if (!copyDirectory(copyFromPath, copyToPath)) {
        MessageBox(nullptr,
                   useChinese ? L"复制文件到目标路径失败！" : L"Failed to copy files to the target path.",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Delete the copyFromPath directory and all its contents
    try {
        fs::remove_all(copyFromPath); // Recursively delete the directory and its contents
    } catch (const std::exception& e) {
        (void)(e);
        MessageBox(nullptr,
                   useChinese ? L"删除源目录时发生错误！" : L"An error occurred while deleting the copyfrom directory!",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Show success message
    MessageBox(nullptr,
               useChinese ? L"ZIP 文件解压并复制成功！" : L"The ZIP file has been extracted and copied successfully!",
               L"ZipUpdater",
               MB_ICONINFORMATION);

    return 0;
}
