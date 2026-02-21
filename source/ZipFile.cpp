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

using namespace std;



ZipFile::ZipFile(const filesystem::path &zipPath)
	: basePath(zipPath)
{
	if(!mz_zip_reader_init_file(&zipFile, basePath.string().c_str(), 0))
		throw runtime_error("Failed to open ZIP file" + zipPath.generic_string());

	// Check whether this zip has a single top-level directory (such as high-dpi.zip/high-dpi)
	filesystem::path topLevel;
	for(const filesystem::path &path : ListFiles("", true, false))
	{
		filesystem::path zipPath = path.lexically_relative(basePath);
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



vector<filesystem::path> ZipFile::ListFiles(const filesystem::path &directory, bool recursive, bool directories)
{
	filesystem::path relative = GetPathInZip(directory);
	vector<filesystem::path> fileList;

  mz_uint count = mz_zip_reader_get_num_files(&zipFile);
  for(mz_uint i = 0; i < count; i++)
  {
    mz_zip_archive_file_stat file_stat;
    if(!mz_zip_reader_file_stat(&zipFile, i, &file_stat))
      continue;
    filesystem::path zipEntry = file_stat.m_filename;
    bool isValidSubtree = Files::IsParent(relative, zipEntry);
    bool isRecursive = distance(zipEntry.begin(), zipEntry.end()) == distance(relative.begin(), relative.end()) + 1;
    if(isValidSubtree && file_stat.m_is_directory == directories && (!isRecursive || recursive))
      fileList.push_back(GetGlobalPath(zipEntry));
  }

	return fileList;
}



bool ZipFile::Exists(const filesystem::path &filePath)
{
	filesystem::path relative = GetPathInZip(filePath);
	string name = relative.generic_string();

  return mz_zip_reader_locate_file(&zipFile, name.c_str(), nullptr, 0) > 0
    || mz_zip_reader_locate_file(&zipFile, name.c_str(), nullptr, 0) > 0;
}



string ZipFile::ReadFile(const filesystem::path &filePath)
{
	const filesystem::path relative = GetPathInZip(filePath);

  const int index = mz_zip_reader_locate_file(&zipFile, relative.generic_string().c_str(), nullptr, 0);
	if(index < 0)
		return {};

  mz_zip_archive_file_stat file_stat;
  if(!mz_zip_reader_file_stat(&zipFile, index, &file_stat) || file_stat.m_uncomp_size == 0) return "";

  const auto buffer = new char[file_stat.m_uncomp_size];
  mz_zip_reader_extract_to_mem(&zipFile, index, buffer, file_stat.m_uncomp_size, 0);
  string contents = buffer;
  delete[] buffer;
	return contents;
}



filesystem::path ZipFile::GetPathInZip(const filesystem::path &path)
{
	filesystem::path relative = path.lexically_relative(basePath);
	if(!topLevelDirectory.empty())
		relative = topLevelDirectory / relative;
	return relative;
}



filesystem::path ZipFile::GetGlobalPath(const filesystem::path &path)
{
	if(path.empty())
		return path;

	// If this zip has a top-level directory, remove it from the path.
	if(!topLevelDirectory.empty())
		return basePath / accumulate(next(path.begin()), path.end(), filesystem::path{}, std::divides{});
	return basePath / path;
}
