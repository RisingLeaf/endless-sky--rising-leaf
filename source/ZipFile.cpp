/* ZipFile.cpp
Copyright (c) 2025 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ZipFile.h"

#include "Files.h"

#include <functional>
#include <numeric>
#include <ranges>





ZipFile::ZipFile(const std::filesystem::path &zipPath)
	: basePath(zipPath)
{
	if(!mz_zip_reader_init_file(&zipFile, basePath.string().c_str(), 0))
		throw std::runtime_error("Failed to open ZIP file" + zipPath.generic_string());

	// Check whether this zip has a single top-level directory (such as high-dpi.zip/high-dpi)
	std::filesystem::path topLevel;
	for(const std::filesystem::path &path : ListFiles("", true, false))
	{
		std::filesystem::path zipPath = path.lexically_relative(basePath);
		if(topLevel.empty())
			topLevel = *zipPath.begin();
		else if(*zipPath.begin() != topLevel)
			return;
	}
	topLevelDirectory = topLevel;
}



ZipFile::~ZipFile()
{
	mz_zip_reader_end(&zipFile);
}



std::vector<std::filesystem::path> ZipFile::ListFiles(const std::filesystem::path &directory, bool recursive, bool directories)
{
	std::filesystem::path relative = GetPathInZip(directory);
	std::vector<std::filesystem::path> fileList;

  mz_uint count = mz_zip_reader_get_num_files(&zipFile);
  for(mz_uint i = 0; i < count; i++)
  {
    mz_zip_archive_file_stat file_stat;
    if(!mz_zip_reader_file_stat(&zipFile, i, &file_stat))
      continue;
    std::filesystem::path zipEntry = file_stat.m_filename;
    bool isValidSubtree = Files::IsParent(relative, zipEntry);
    bool isRecursive = std::distance(zipEntry.begin(), zipEntry.end()) == std::distance(relative.begin(), relative.end()) + 1;
    if(isValidSubtree && file_stat.m_is_directory == directories && (!isRecursive || recursive))
      fileList.push_back(GetGlobalPath(zipEntry));
  }

	return fileList;
}



bool ZipFile::Exists(const std::filesystem::path &filePath)
{
	std::filesystem::path relative = GetPathInZip(filePath);
	std::string name = relative.generic_string();

  return mz_zip_reader_locate_file(&zipFile, name.c_str(), nullptr, 0) > 0
    || mz_zip_reader_locate_file(&zipFile, name.c_str(), nullptr, 0) > 0;
}



std::string ZipFile::ReadFile(const std::filesystem::path &filePath)
{
	const std::filesystem::path relative = GetPathInZip(filePath);

  const int index = mz_zip_reader_locate_file(&zipFile, relative.generic_string().c_str(), nullptr, 0);
	if(index < 0)
		return {};

  mz_zip_archive_file_stat file_stat;
  if(!mz_zip_reader_file_stat(&zipFile, index, &file_stat) || file_stat.m_uncomp_size == 0) return "";

  const auto buffer = new char[file_stat.m_uncomp_size];
  mz_zip_reader_extract_to_mem(&zipFile, index, buffer, file_stat.m_uncomp_size, 0);
  std::string contents = buffer;
  delete[] buffer;
	return contents;
}



std::filesystem::path ZipFile::GetPathInZip(const std::filesystem::path &path)
{
	std::filesystem::path relative = path.lexically_relative(basePath);
	if(!topLevelDirectory.empty())
		relative = topLevelDirectory / relative;
	return relative;
}



std::filesystem::path ZipFile::GetGlobalPath(const std::filesystem::path &path)
{
	if(path.empty())
		return path;

	// If this zip has a top-level directory, remove it from the path.
	if(!topLevelDirectory.empty())
		return basePath / accumulate(std::next(path.begin()), path.end(), std::filesystem::path{}, std::divides{});
	return basePath / path;
}
