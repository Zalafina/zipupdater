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

std::string ConvertWStringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, const_cast<LPWSTR>(wstr.c_str()), -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed - 1, 0); // -1 to exclude the null terminator
    WideCharToMultiByte(CP_UTF8, 0, const_cast<LPWSTR>(wstr.c_str()), -1, &strTo[0], size_needed - 1, NULL, NULL); // -1 to exclude the null terminator
    return strTo;
}

std::wstring ConvertStringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstrTo(size_needed - 1, 0); // -1 to exclude the null terminator
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstrTo[0], size_needed - 1); // -1 to exclude the null terminator
    return wstrTo;
}

// Function to convert GBK encoded std::string to UTF-8 std::string
std::string gbkToUtf8(const std::string &gbkStr) {
    // Step 1: Convert GBK to UTF-16 (wstring)
    int wstrLen = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, nullptr, 0);
    if (wstrLen == 0) {
        return ""; // Conversion failed
    }
    std::wstring wstr(wstrLen, 0);
    MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, &wstr[0], wstrLen);

    // Step 2: Convert UTF-16 to UTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len == 0) {
        return ""; // Conversion failed
    }
    std::string utf8Str(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], utf8Len, nullptr, nullptr);

    return utf8Str;
}

bool extractZip(const std::string &zipFile, const std::string &outputDir) {
    void *reader = nullptr;
    int32_t err = MZ_OK;
    int32_t err_close = MZ_OK;

#ifdef DEBUG_LOG_ON
    std::cout << "Extracting archive: " << zipFile << std::endl;
#endif

    reader = mz_zip_reader_create();
    if (!reader) {
#ifdef DEBUG_LOG_ON
        std::cerr << "Failed to create ZIP reader." << std::endl;
#endif
        return false;
    }

    mz_zip_reader_set_encoding(reader, MZ_ENCODING_UTF8);

    err = mz_zip_reader_open_file(reader, zipFile.c_str());
    if (err != MZ_OK) {
#ifdef DEBUG_LOG_ON
        std::cerr << "Error opening ZIP file: " << zipFile << " (error code: " << err << ")" << std::endl;
#endif
        mz_zip_reader_delete(&reader);
        return false;
    }

    err = mz_zip_reader_save_all(reader, outputDir.c_str());
    if (err == MZ_END_OF_LIST) {
#ifdef DEBUG_LOG_ON
        std::cout << "No files found in the archive." << std::endl;
#endif
        err = MZ_OK;
    } else if (err != MZ_OK) {
#ifdef DEBUG_LOG_ON
        std::cerr << "Error saving entries to disk: " << zipFile << " (error code: " << err << ")" << std::endl;
#endif
    }

    err_close = mz_zip_reader_close(reader);
    if (err_close != MZ_OK) {
#ifdef DEBUG_LOG_ON
        std::cerr << "Error closing ZIP reader (error code: " << err_close << ")" << std::endl;
#endif
        err = err_close;
    }

    mz_zip_reader_delete(&reader);

    return (err == MZ_OK);
}

bool copyDirectory(const fs::path& source, const fs::path& destination) {
    if (!fs::exists(source) || !fs::is_directory(source)) {
#ifdef DEBUG_LOG_ON
        std::cerr << "Error: Source path does not exist or is not a directory." << std::endl;
#endif
        return false;
    }

    try {
        fs::create_directories(destination);
        fs::copy(source, destination, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
#ifdef DEBUG_LOG_ON
        std::cout << "Directory copied successfully!" << std::endl;
#endif
    } catch (fs::filesystem_error& e) {
        (void)(e);
#ifdef DEBUG_LOG_ON
        std::cerr << "Error: " << e.what() << std::endl;
#endif
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    // Set DPI awareness for high-DPI displays
    SetProcessDPIAware();

    // Check for the correct number of arguments
    if ((argc < 4) || (argc > 6)) {
        MessageBox(nullptr, L"Usage: zipupdater.exe <zip_path> <copyfrom_path> <copyto_path> [executable_path] [--chinese]", L"ZipUpdater", MB_ICONWARNING);
        return 1;
    }

    // Set the working directory to the executable's directory
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH); // Get the full path of the executable
    std::wstring exeDir = fs::path(exePath).parent_path().wstring(); // Get the directory of the executable
    SetCurrentDirectory(exeDir.c_str()); // Set it as the current working directory

    // Parse arguments
    std::string zipFilePath_ori = argv[1];
    std::string zipFilePath = gbkToUtf8(zipFilePath_ori);
    std::string copyFromPath = argv[2];
    std::string copyToPath = argv[3];
    std::string executablePath;
    bool useChinese = (argc == 5 && std::string(argv[4]) == "--chinese");

    if (argc == 5) {
        if (std::string(argv[4]) == "--chinese") {
            useChinese = true;
        } else {
            executablePath = argv[4];
        }
    } else if (argc == 6) {
        executablePath = argv[4];
        useChinese = (std::string(argv[5]) == "--chinese");
    }

    // Check if the ZIP file exists
    if (!fs::exists(zipFilePath_ori)) {
        MessageBox(nullptr,
                   useChinese ? L"ZIP 文件不存在！" : L"The ZIP file does not exist!",
                   L"ZipUpdater",
                   MB_ICONWARNING);
        return 1;
    }

    // If zipupdater_tempdir exists, remove all its contents. Otherwise, create an empty directory
    const std::string tempDir = "zipupdater_tempdir";
    try {
        if (fs::exists(tempDir)) {
            fs::remove_all(tempDir); // Remove all contents in zipupdater_tempdir
        }
        fs::create_directory(tempDir); // Create an empty zipupdater_tempdir
    } catch (const std::exception& e) {
        (void)(e);
        MessageBox(nullptr,
                   useChinese ? L"创建临时目录时发生错误！" : L"An error occurred while creating the temporary directory!",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Update copyFromPath to zipupdater_tempdir/<copyFromPath>
    copyFromPath = tempDir + "/" + copyFromPath;

    // Extract the ZIP file
    std::string currentPath = "zipupdater_tempdir";
    if (!extractZip(zipFilePath, currentPath)) {
        if (fs::exists(tempDir)) {
            fs::remove_all(tempDir); // Remove all contents in zipupdater_tempdir
        }
        MessageBox(nullptr,
                   useChinese ? L"解压 ZIP 文件时发生错误！" : L"An error occurred while extracting the ZIP file!",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Copy the files
    if (!copyDirectory(copyFromPath, copyToPath)) {
        if (fs::exists(tempDir)) {
            fs::remove_all(tempDir); // Remove all contents in zipupdater_tempdir
        }
        MessageBox(nullptr,
                   useChinese ? L"复制文件到目标路径失败！" : L"Failed to copy files to the target path.",
                   L"ZipUpdater",
                   MB_ICONERROR);
        return 1;
    }

    // Delete the temp directory and all its contents
    try {
        if (fs::exists(tempDir)) {
            fs::remove_all(tempDir); // Remove all contents in zipupdater_tempdir
        }
    } catch (const std::exception& e) {
        (void)(e);
        MessageBox(nullptr,
                   useChinese ? L"删除临时目录时发生错误！" : L"An error occurred while deleting the temporary directory!",
                   L"ZipUpdater",
                   MB_ICONERROR);
    }

    // Run the executable if provided
    if (!executablePath.empty()) {
        if (!fs::exists(executablePath)) {
            MessageBox(nullptr,
                       useChinese ? L"指定的可执行文件不存在！" : L"The executable file does not exist!",
                       L"ZipUpdater",
                       MB_ICONWARNING);
            return 1;
        }

        std::wstring executablePathW = ConvertStringToWString(executablePath);
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcess(nullptr, const_cast<LPWSTR>(executablePathW.c_str()), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
            MessageBox(nullptr,
                       useChinese ? L"运行可执行文件时发生错误！" : L"An error occurred while running the executable!",
                       L"ZipUpdater",
                       MB_ICONERROR);
        }
        else {
            // Close handles immediately to detach the process
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    // Show success message
    MessageBox(nullptr,
               useChinese ? L"ZIP 文件解压并复制成功！" : L"The ZIP file has been extracted and copied successfully!",
               L"ZipUpdater",
               MB_ICONINFORMATION);

    return 0;
}
