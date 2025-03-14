/*
 * Copyright (C) 2007-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FileSystem.h"

#include "FileMetadata.h"
#include "NotImplemented.h"
#include "PathWalker.h"
#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sys/stat.h>
#include <windows.h>
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/win/WCharStringExtras.h>

namespace WebCore {

namespace FileSystem {

static const ULONGLONG kSecondsFromFileTimeToTimet = 11644473600;

static bool getFindData(String path, WIN32_FIND_DATAW& findData)
{
    HANDLE handle = FindFirstFileW(stringToNullTerminatedWChar(path).data(), &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    FindClose(handle);
    return true;
}

static bool getFileSizeFromFindData(const WIN32_FIND_DATAW& findData, long long& size)
{
    ULARGE_INTEGER fileSize;
    fileSize.HighPart = findData.nFileSizeHigh;
    fileSize.LowPart = findData.nFileSizeLow;

    if (fileSize.QuadPart > static_cast<ULONGLONG>(std::numeric_limits<long long>::max()))
        return false;

    size = fileSize.QuadPart;
    return true;
}

static bool getFileSizeFromByHandleFileInformationStructure(const BY_HANDLE_FILE_INFORMATION& fileInformation, long long& size)
{
    ULARGE_INTEGER fileSize;
    fileSize.HighPart = fileInformation.nFileSizeHigh;
    fileSize.LowPart = fileInformation.nFileSizeLow;

    if (fileSize.QuadPart > static_cast<ULONGLONG>(std::numeric_limits<long long>::max()))
        return false;

    size = fileSize.QuadPart;
    return true;
}

static void getFileCreationTimeFromFindData(const WIN32_FIND_DATAW& findData, time_t& time)
{
    ULARGE_INTEGER fileTime;
    fileTime.HighPart = findData.ftCreationTime.dwHighDateTime;
    fileTime.LowPart = findData.ftCreationTime.dwLowDateTime;

    // Information about converting time_t to FileTime is available at http://msdn.microsoft.com/en-us/library/ms724228%28v=vs.85%29.aspx
    time = fileTime.QuadPart / 10000000 - kSecondsFromFileTimeToTimet;
}


static void getFileModificationTimeFromFindData(const WIN32_FIND_DATAW& findData, time_t& time)
{
    ULARGE_INTEGER fileTime;
    fileTime.HighPart = findData.ftLastWriteTime.dwHighDateTime;
    fileTime.LowPart = findData.ftLastWriteTime.dwLowDateTime;

    // Information about converting time_t to FileTime is available at http://msdn.microsoft.com/en-us/library/ms724228%28v=vs.85%29.aspx
    time = fileTime.QuadPart / 10000000 - kSecondsFromFileTimeToTimet;
}

bool getFileSize(const String& path, long long& size)
{
    WIN32_FIND_DATAW findData;
    if (!getFindData(path, findData))
        return false;

    return getFileSizeFromFindData(findData, size);
}

bool getFileSize(PlatformFileHandle fileHandle, long long& size)
{
    BY_HANDLE_FILE_INFORMATION fileInformation;
    if (!::GetFileInformationByHandle(fileHandle, &fileInformation))
        return false;

    return getFileSizeFromByHandleFileInformationStructure(fileInformation, size);
}

bool getFileModificationTime(const String& path, time_t& time)
{
    WIN32_FIND_DATAW findData;
    if (!getFindData(path, findData))
        return false;

    getFileModificationTimeFromFindData(findData, time);
    return true;
}

bool getFileCreationTime(const String& path, time_t& time)
{
    WIN32_FIND_DATAW findData;
    if (!getFindData(path, findData))
        return false;

    getFileCreationTimeFromFindData(findData, time);
    return true;
}

static String getFinalPathName(const String& path)
{
    auto handle = openFile(path, FileOpenMode::Read);
    if (!isHandleValid(handle))
        return String();

    Vector<UChar> buffer(MAX_PATH);
    if (::GetFinalPathNameByHandleW(handle, buffer.data(), buffer.size(), VOLUME_NAME_NT) >= MAX_PATH) {
        closeFile(handle);
        return String();
    }
    closeFile(handle);

    buffer.shrink(wcslen(buffer.data()));
    return String::adopt(WTFMove(buffer));
}

static inline bool isSymbolicLink(WIN32_FIND_DATAW findData)
{
    return findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT && findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK;
}

static FileMetadata::Type toFileMetadataType(WIN32_FIND_DATAW findData)
{
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return FileMetadata::Type::Directory;
    if (isSymbolicLink(findData))
        return FileMetadata::Type::SymbolicLink;
    return FileMetadata::Type::File;
}

static std::optional<FileMetadata> findDataToFileMetadata(WIN32_FIND_DATAW findData)
{
    long long length;
    if (!getFileSizeFromFindData(findData, length))
        return std::nullopt;

    time_t modificationTime;
    getFileModificationTimeFromFindData(findData, modificationTime);

    return FileMetadata {
        static_cast<double>(modificationTime),
        length,
        static_cast<bool>(findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN),
        toFileMetadataType(findData)
    };
}

std::optional<FileMetadata> fileMetadata(const String& path)
{
    WIN32_FIND_DATAW findData;
    if (!getFindData(path, findData))
        return std::nullopt;

    return findDataToFileMetadata(findData);
}

std::optional<FileMetadata> fileMetadataFollowingSymlinks(const String& path)
{
    WIN32_FIND_DATAW findData;
    if (!getFindData(path, findData))
        return std::nullopt;

    if (isSymbolicLink(findData)) {
        String targetPath = getFinalPathName(path);
        if (targetPath.isNull())
            return std::nullopt;
        if (!getFindData(targetPath, findData))
            return std::nullopt;
    }

    return findDataToFileMetadata(findData);
}

bool createSymbolicLink(const String& targetPath, const String& symbolicLinkPath)
{
    return !::CreateSymbolicLinkW(stringToNullTerminatedWChar(symbolicLinkPath).data(), stringToNullTerminatedWChar(targetPath).data(), 0);
}

bool fileExists(const String& path)
{
    WIN32_FIND_DATAW findData;
    return getFindData(path, findData);
}

bool deleteFile(const String& path)
{
    String filename = path;
    return !!DeleteFileW(stringToNullTerminatedWChar(filename).data());
}

bool deleteEmptyDirectory(const String& path)
{
    String filename = path;
    return !!RemoveDirectoryW(stringToNullTerminatedWChar(filename).data());
}

bool moveFile(const String& oldPath, const String& newPath)
{
    String oldFilename = oldPath;
    String newFilename = newPath;
    return !!::MoveFileEx(stringToNullTerminatedWChar(oldFilename).data(), stringToNullTerminatedWChar(newFilename).data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
}

String pathByAppendingComponent(const String& path, const String& component)
{
    Vector<UChar> buffer(MAX_PATH);

    if (path.length() + 1 > buffer.size())
        return String();

    StringView(path).getCharactersWithUpconvert(buffer.data());
    buffer[path.length()] = '\0';

    if (!PathAppendW(buffer.data(), stringToNullTerminatedWChar(component).data()))
        return String();

    buffer.shrink(wcslen(buffer.data()));

    return String::adopt(WTFMove(buffer));
}

String pathByAppendingComponents(StringView path, const Vector<StringView>& components)
{
    String result = path.toString();
    for (auto& component : components)
        result = pathByAppendingComponent(result, component.toString());
    return result;
}

#if !USE(CF)

CString fileSystemRepresentation(const String& path)
{
    auto upconvertedCharacters = path.upconvertedCharacters();

    const UChar* characters = upconvertedCharacters;
    int size = WideCharToMultiByte(CP_ACP, 0, characters, path.length(), 0, 0, 0, 0) - 1;

    char* buffer;
    CString string = CString::newUninitialized(size, buffer);

    WideCharToMultiByte(CP_ACP, 0, characters, path.length(), buffer, size, 0, 0);

    return string;
}

#endif // !USE(CF)

bool makeAllDirectories(const String& path)
{
    String fullPath = path;
    if (SHCreateDirectoryEx(0, stringToNullTerminatedWChar(fullPath).data(), 0) != ERROR_SUCCESS) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) {
            LOG_ERROR("Failed to create path %s", path.ascii().data());
            return false;
        }
    }
    return true;
}

String homeDirectoryPath()
{
    notImplemented();
    return "";
}

String pathGetFileName(const String& path)
{
    return nullTerminatedWCharToString(::PathFindFileName(stringToNullTerminatedWChar(path).data()));
}

String directoryName(const String& path)
{
    String name = path.left(path.length() - pathGetFileName(path).length());
    if (name.characterStartingAt(name.length() - 1) == '\\') {
        // Remove any trailing "\".
        name.truncate(name.length() - 1);
    }
    return name;
}

static String bundleName()
{
    static const NeverDestroyed<String> name = [] {
        String name { "WebKit"_s };

#if USE(CF)
        if (CFBundleRef bundle = CFBundleGetMainBundle()) {
            if (CFTypeRef bundleExecutable = CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleExecutableKey)) {
                if (CFGetTypeID(bundleExecutable) == CFStringGetTypeID())
                    name = reinterpret_cast<CFStringRef>(bundleExecutable);
            }
        }
#endif

        return name;
    }();

    return name;
}

static String storageDirectory(DWORD pathIdentifier)
{
    Vector<UChar> buffer(MAX_PATH);
    if (FAILED(SHGetFolderPathW(0, pathIdentifier | CSIDL_FLAG_CREATE, 0, 0, buffer.data())))
        return String();
    buffer.resize(wcslen(buffer.data()));
    String directory = String::adopt(WTFMove(buffer));

    directory = pathByAppendingComponent(directory, "Apple Computer\\" + bundleName());
    if (!makeAllDirectories(directory))
        return String();

    return directory;
}

static String cachedStorageDirectory(DWORD pathIdentifier)
{
    static HashMap<DWORD, String> directories;

    HashMap<DWORD, String>::iterator it = directories.find(pathIdentifier);
    if (it != directories.end())
        return it->value;

    String directory = storageDirectory(pathIdentifier);
    directories.add(pathIdentifier, directory);

    return directory;
}

static String generateTemporaryPath(const Function<bool(const String&)>& action)
{
    wchar_t tempPath[MAX_PATH];
    int tempPathLength = ::GetTempPathW(WTF_ARRAY_LENGTH(tempPath), tempPath);
    if (tempPathLength <= 0 || tempPathLength > WTF_ARRAY_LENGTH(tempPath))
        return String();

    String proposedPath;
    do {
        wchar_t tempFile[] = L"XXXXXXXX.tmp"; // Use 8.3 style name (more characters aren't helpful due to 8.3 short file names)
        const int randomPartLength = 8;
        cryptographicallyRandomValues(tempFile, randomPartLength * sizeof(wchar_t));

        // Limit to valid filesystem characters, also excluding others that could be problematic, like punctuation.
        // don't include both upper and lowercase since Windows file systems are typically not case sensitive.
        const char validChars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        for (int i = 0; i < randomPartLength; ++i)
            tempFile[i] = validChars[tempFile[i] % (sizeof(validChars) - 1)];

        ASSERT(wcslen(tempFile) == WTF_ARRAY_LENGTH(tempFile) - 1);

        proposedPath = pathByAppendingComponent(nullTerminatedWCharToString(tempPath), nullTerminatedWCharToString(tempFile));
        if (proposedPath.isEmpty())
            break;
    } while (!action(proposedPath));

    return proposedPath;
}

String openTemporaryFile(const String&, PlatformFileHandle& handle)
{
    handle = INVALID_HANDLE_VALUE;

    String proposedPath = generateTemporaryPath([&handle](const String& proposedPath) {
        // use CREATE_NEW to avoid overwriting an existing file with the same name
        handle = ::CreateFileW(stringToNullTerminatedWChar(proposedPath).data(), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);

        return isHandleValid(handle) || GetLastError() == ERROR_ALREADY_EXISTS;
    });

    if (!isHandleValid(handle))
        return String();

    return proposedPath;
}

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
{
    DWORD desiredAccess = 0;
    DWORD creationDisposition = 0;
    DWORD shareMode = 0;
    switch (mode) {
    case FileOpenMode::Read:
        desiredAccess = GENERIC_READ;
        creationDisposition = OPEN_EXISTING;
        shareMode = FILE_SHARE_READ;
        break;
    case FileOpenMode::Write:
        desiredAccess = GENERIC_WRITE;
        creationDisposition = CREATE_ALWAYS;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    String destination = path;
    return CreateFile(stringToNullTerminatedWChar(destination).data(), desiredAccess, shareMode, 0, creationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
}

void closeFile(PlatformFileHandle& handle)
{
    if (isHandleValid(handle)) {
        ::CloseHandle(handle);
        handle = invalidPlatformFileHandle;
    }
}

long long seekFile(PlatformFileHandle handle, long long offset, FileSeekOrigin origin)
{
    DWORD moveMethod = FILE_BEGIN;

    if (origin == FileSeekOrigin::Current)
        moveMethod = FILE_CURRENT;
    else if (origin == FileSeekOrigin::End)
        moveMethod = FILE_END;

    LARGE_INTEGER largeOffset;
    largeOffset.QuadPart = offset;

    largeOffset.LowPart = SetFilePointer(handle, largeOffset.LowPart, &largeOffset.HighPart, moveMethod);

    if (largeOffset.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        return -1;

    return largeOffset.QuadPart;
}

int writeToFile(PlatformFileHandle handle, const char* data, int length)
{
    if (!isHandleValid(handle))
        return -1;

    DWORD bytesWritten;
    bool success = WriteFile(handle, data, length, &bytesWritten, 0);

    if (!success)
        return -1;
    return static_cast<int>(bytesWritten);
}

int readFromFile(PlatformFileHandle handle, char* data, int length)
{
    if (!isHandleValid(handle))
        return -1;

    DWORD bytesRead;
    bool success = ::ReadFile(handle, data, length, &bytesRead, 0);

    if (!success)
        return -1;
    return static_cast<int>(bytesRead);
}

bool hardLinkOrCopyFile(const String& source, const String& destination)
{
    return !!::CopyFile(stringToNullTerminatedWChar(source).data(), stringToNullTerminatedWChar(destination).data(), TRUE);
}

String localUserSpecificStorageDirectory()
{
    return cachedStorageDirectory(CSIDL_LOCAL_APPDATA);
}

String roamingUserSpecificStorageDirectory()
{
    return cachedStorageDirectory(CSIDL_APPDATA);
}

Vector<String> listDirectory(const String& directory, const String& filter)
{
    Vector<String> entries;

    PathWalker walker(directory, filter);
    if (!walker.isValid())
        return entries;

    do {
        if (walker.data().dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
            && (!wcscmp(walker.data().cFileName, L".") || !wcscmp(walker.data().cFileName, L"..")))
            continue;

        entries.append(directory + "\\" + reinterpret_cast<const UChar*>(walker.data().cFileName));
    } while (walker.step());

    return entries;
}

bool getVolumeFreeSpace(const String&, uint64_t&)
{
    notImplemented();
    return false;
}

std::optional<int32_t> getFileDeviceId(const CString& fsFile)
{
    auto handle = openFile(fsFile.data(), FileOpenMode::Read);
    if (!isHandleValid(handle))
        return std::nullopt;

    BY_HANDLE_FILE_INFORMATION fileInformation = { };
    if (!::GetFileInformationByHandle(handle, &fileInformation)) {
        closeFile(handle);
        return std::nullopt;
    }

    closeFile(handle);

    return fileInformation.dwVolumeSerialNumber;
}

String realPath(const String& filePath)
{
    return getFinalPathName(filePath);
}

String createTemporaryDirectory()
{
    return generateTemporaryPath([](const String& proposedPath) {
        return makeAllDirectories(proposedPath);
    });
}

bool deleteNonEmptyDirectory(const String& directoryPath)
{
    SHFILEOPSTRUCT deleteOperation = {
        nullptr,
        FO_DELETE,
        stringToNullTerminatedWChar(directoryPath).data(),
        L"",
        FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
        false,
        0,
        L""
    };
    return !SHFileOperation(&deleteOperation);
}

} // namespace FileSystem
} // namespace WebCore
