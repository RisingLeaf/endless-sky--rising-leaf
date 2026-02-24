/* Files.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Files.h"

#include "Logger.h"
#include "ZipFile.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>



namespace {
	std::filesystem::path resources;
	std::filesystem::path config;

	std::filesystem::path dataPath;
	std::filesystem::path imagePath;
	std::filesystem::path soundPath;
	std::filesystem::path savePath;
	std::filesystem::path userPluginPath;
	std::filesystem::path globalPluginPath;
	std::filesystem::path testPath;

	std::shared_ptr<std::iostream> errorLog;

	// Open the given folder in a separate window.
	void OpenFolder(const std::filesystem::path &path)
	{
		// TODO: Remove SDL version check after Ubuntu 20.04 reaches end of life
#if SDL_VERSION_ATLEAST(2, 0, 14)
		if(SDL_OpenURL(("file://" + path.string()).c_str()))
			Logger::LogError("Warning: SDL_OpenURL failed with \"" + std::string(SDL_GetError()) + "\"");
#elif defined(__linux__)
		// Some supported distributions do not have an up-to-date SDL.
		cout.flush();
		if(int result = WEXITSTATUS(system(("xdg-open file://" + path.string()).c_str())))
			Logger::LogError("Warning: xdg-open failed with error code " + to_string(result) + ".");
#else
#warning SDL 2.0.14 or higher is needed for opening folders!
		Logger::LogError("Warning: No handler found to open \"" + path + "\" in a new window.");
#endif
	}

	/// The open zip files per thread. Since ZLIB doesn't support multithreaded access on the same zip handle,
	/// each file is opened multiple times on demand.
	thread_local std::map<std::filesystem::path, std::shared_ptr<ZipFile>> OPEN_ZIP_FILES;

	std::shared_ptr<ZipFile> GetZipFile(const std::filesystem::path &filePath)
	{
		/// Check if this zip is already open on this thread.
		for(auto &[zipPath, file] : OPEN_ZIP_FILES)
			if(Files::IsParent(zipPath, filePath))
				return file;

		/// If not, open the zip file.
		std::filesystem::path zipPath = filePath;
		while(!exists(zipPath))
		{
			if(!zipPath.has_parent_path() || zipPath.parent_path() == zipPath)
				return {};
			zipPath = zipPath.parent_path();
		}
		if(zipPath.extension() == ".zip" && is_regular_file(zipPath))
		{
			/// Limit the number of open zip files to one per thread to avoid having too many files open.
			OPEN_ZIP_FILES.clear();
			return OPEN_ZIP_FILES.emplace(zipPath, std::make_shared<ZipFile>(zipPath)).first->second;
		}

		return {};
	}
}



void Files::Init(const char * const *argv)
{
	// Parse the command line arguments to see if the user has specified
	// different directories to use.
	for(const char * const *it = argv + 1; *it; ++it)
	{
		std::string arg = *it;
		if((arg == "-r" || arg == "--resources") && *++it)
			resources = *it;
		else if((arg == "-c" || arg == "--config") && *++it)
			config = *it;

	}

	if(resources.empty())
	{
		// Find the path to the resource directory. This will depend on the
		// operating system, and can be overridden by a command line argument.
		const char *basePath = SDL_GetBasePath();
		if(!basePath)
			throw std::runtime_error("Unable to get path to resource directory!");
		resources = basePath;

		if(Exists(resources))
			resources = std::filesystem::canonical(resources);

#if defined __linux__ || defined __FreeBSD__ || defined __DragonFly__
		// Special case, for Linux: the resource files are not in the same place as
		// the executable, but are under the same prefix (/usr or /usr/local).
		// When used as an iterator, a trailing / will create an empty item at
		// the end, so parent paths do not include it.
		static const std::filesystem::path LOCAL_PATH = "/usr/local";
		static const std::filesystem::path STANDARD_PATH = "/usr";
		static const std::filesystem::path RESOURCE_PATH = "share/games/endless-sky/";

		if(IsParent(LOCAL_PATH, resources))
			resources = LOCAL_PATH / RESOURCE_PATH;
		else if(IsParent(STANDARD_PATH, resources))
			resources = STANDARD_PATH / RESOURCE_PATH;
#endif
	}
	// If the resources are not here, search in the directories containing this
	// one. This allows, for example, a Mac app that does not actually have the
	// resources embedded within it.
	while(!Exists(resources / "credits.txt"))
	{
		if(!resources.has_parent_path() || resources.parent_path() == resources)
			throw std::runtime_error("Unable to find the resource directories!");
		resources = resources.parent_path();
	}
	dataPath = resources / "data";
	imagePath = resources / "images";
	soundPath = resources / "sounds";
	globalPluginPath = resources / "plugins";

	if(config.empty())
	{
		// Create the directory for the saved games, preferences, etc., if necessary.
		char *str = SDL_GetPrefPath(nullptr, "endless-sky");
		if(!str)
			throw std::runtime_error("Unable to get path to config directory!");
		config = str;
		SDL_free(str);
	}

	if(!Exists(config))
		throw std::runtime_error("Unable to create config directory!");

	config = std::filesystem::canonical(config);

	savePath = config / "saves";
	CreateFolder(savePath);

	// Create the "plugins" directory if it does not yet exist, so that it is
	// clear to the user where plugins should go.
	userPluginPath = config / "plugins";
	CreateFolder(userPluginPath);

	// Check that all the directories exist.
	if(!Exists(dataPath) || !Exists(imagePath) || !Exists(soundPath))
		throw std::runtime_error("Unable to find the resource directories!");
	if(!Exists(savePath))
		throw std::runtime_error("Unable to create save directory!");
	if(!Exists(userPluginPath))
		throw std::runtime_error("Unable to create plugins directory!");
}



const std::filesystem::path &Files::Resources()
{
	return resources;
}



const std::filesystem::path &Files::Config()
{
	return config;
}



const std::filesystem::path &Files::Data()
{
	return dataPath;
}



const std::filesystem::path &Files::Images()
{
	return imagePath;
}



const std::filesystem::path &Files::Sounds()
{
	return soundPath;
}



const std::filesystem::path &Files::Saves()
{
	return savePath;
}



const std::filesystem::path &Files::UserPlugins()
{
	return userPluginPath;
}



const std::filesystem::path &Files::GlobalPlugins()
{
	return globalPluginPath;
}



const std::filesystem::path &Files::Tests()
{
	return testPath;
}



std::vector<std::filesystem::path> Files::List(const std::filesystem::path &directory)
{
	std::vector<std::filesystem::path> list;

	if(!Exists(directory) || !is_directory(directory))
	{
		// Check if the requested file is in a known zip.
		std::shared_ptr<ZipFile> zip = GetZipFile(directory);
		if(zip)
		{
			list = zip->ListFiles(directory, false, false);
			sort(list.begin(), list.end());
		}
		return list;
	}


	for(const auto &entry : std::filesystem::directory_iterator(directory))
		if(entry.is_regular_file())
			list.emplace_back(entry);

	sort(list.begin(), list.end());

	return list;
}



// Get a list of any directories in the given directory.
std::vector<std::filesystem::path> Files::ListDirectories(const std::filesystem::path &directory)
{
	std::vector<std::filesystem::path> list;

	if(!Exists(directory) || !is_directory(directory))
	{
		// Check if the requested file is in a known zip.
		std::shared_ptr<ZipFile> zip = GetZipFile(directory);
		if(zip)
		{
			list = zip->ListFiles(directory, false, true);
			sort(list.begin(), list.end());
		}
		return list;
	}

	for(const auto &entry : std::filesystem::directory_iterator(directory))
		if(entry.is_directory())
			list.emplace_back(entry);

	sort(list.begin(), list.end());
	return list;
}



std::vector<std::filesystem::path> Files::RecursiveList(const std::filesystem::path &directory)
{
	std::vector<std::filesystem::path> list;
	if(!Exists(directory) || !is_directory(directory))
	{
		// Check if the requested file is in a known zip.
		std::shared_ptr<ZipFile> zip = GetZipFile(directory);
		if(zip)
		{
			list = zip->ListFiles(directory, true, false);
			sort(list.begin(), list.end());
		}
		return list;
	}

	for(const auto &entry : std::filesystem::recursive_directory_iterator(directory))
		if(entry.is_regular_file())
			list.emplace_back(entry);

	sort(list.begin(), list.end());
	return list;
}



bool Files::Exists(const std::filesystem::path &filePath)
{
	if(exists(filePath))
		return true;

	std::shared_ptr<ZipFile> zip = GetZipFile(filePath);
	if(zip)
		return zip->Exists(filePath);
	return false;
}



std::filesystem::file_time_type Files::Timestamp(const std::filesystem::path &filePath)
{
	return last_write_time(filePath);
}



bool Files::Copy(const std::filesystem::path &from, const std::filesystem::path &to)
{
#ifdef _WIN32
	// Due to a mingw bug, the overwrite_existing flag is not respected on Windows.
	// TODO: remove once it is fixed.
	if(Exists(to))
		Delete(to);
#endif
	try {
		copy(from, to, std::filesystem::copy_options::overwrite_existing);
	}
	catch(...)
	{
		return false;
	}
	return true;
}



void Files::Move(const std::filesystem::path &from, const std::filesystem::path &to)
{
	rename(from, to);
}



void Files::Delete(const std::filesystem::path &filePath)
{
	remove_all(filePath);
}



// Get the filename from a path.
std::string Files::Name(const std::filesystem::path &path)
{
	return path.filename().string();
}



bool Files::IsParent(const std::filesystem::path &parent, const std::filesystem::path &child)
{
	if(std::distance(child.begin(), child.end()) < std::distance(parent.begin(), parent.end()))
		return false;
	return std::equal(parent.begin(), parent.end(), child.begin());
}



std::shared_ptr<std::iostream> Files::Open(const std::filesystem::path &path, bool write)
{
	if(!exists(path) && !write)
	{
		// Writing to a zip is not supported.
		std::shared_ptr<ZipFile> zip = GetZipFile(path);
		if(zip)
			return std::shared_ptr<std::iostream>(new std::stringstream(zip->ReadFile(path), std::ios::in | std::ios::binary));
		return {};
	}

	if(write)
#ifdef _WIN32
		return shared_ptr<iostream>{new fstream{path, ios::out}};
#else
		return std::shared_ptr<std::iostream>{new std::fstream{path, std::ios::out | std::ios::binary}};
#endif
	return std::shared_ptr<std::iostream>{new std::fstream{path, std::ios::in | std::ios::binary}};
}



std::string Files::Read(const std::filesystem::path &path)
{
	return Read(Open(path));
}



std::string Files::Read(std::shared_ptr<std::iostream> file)
{
	if(!file)
		return "";
	return std::string{std::istreambuf_iterator<char>{*file}, {}};
}



void Files::Write(const std::filesystem::path &path, const std::string &data)
{
	Write(Open(path, true), data);
}



void Files::Write(std::shared_ptr<std::iostream> file, const std::string &data)
{
	if(!file)
		return;
	*file << data;
	file->flush();
}



void Files::CreateFolder(const std::filesystem::path &path)
{
	if(Exists(path))
		return;

	if(std::filesystem::create_directory(path))
		std::filesystem::permissions(path, std::filesystem::perms(std::filesystem::perms::owner_all));
	else
		throw std::runtime_error("Error creating directory!");
}



// Open this user's plugins directory in their native file explorer.
void Files::OpenUserPluginFolder()
{
	OpenFolder(userPluginPath);
}



// Open this user's save file directory in their native file explorer.
void Files::OpenUserSavesFolder()
{
	OpenFolder(savePath);
}



void Files::LogErrorToFile(const std::string &message)
{
	if(!errorLog)
	{
		errorLog = Open(config / "errors.txt", true);
		if(!errorLog)
		{
			std::cerr << "Unable to create \"errors.txt\" " << (config.empty()
				? "in current directory" : "in \"" + config.string() + "\"") << std::endl;
			return;
		}
	}

	Write(errorLog, message);
	*errorLog << std::endl;
}
